#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

#define BUFFERSIZE (64*1024)

static char *path_del = "/";
static char *column_del = "\t";
static char *array_prefix = "@";
static char *object_prefix = "%";
static FILE *input, *output;

static const char *mystrndup (const char *str, int len)
{
	static char buffer[BUFFERSIZE];
	int lenx = len < BUFFERSIZE - 1 ? len : BUFFERSIZE - 1;
	memcpy(buffer, str, lenx);
	buffer[lenx] = '\0';
	return buffer;
}

static const char *next_token (void)
{
	static char buffer[BUFFERSIZE];
	static int buffer_head = 0, buffer_tail = 0, eof = 0;
	const char *retval;

	if (buffer_tail - buffer_head < BUFFERSIZE / 2 && eof == 0) {
		if (buffer_head != 0) {
			memmove(buffer, buffer + buffer_head, buffer_tail - buffer_head);
			buffer_tail -= buffer_head;
			buffer_head = 0;
		}
		do {
			int length = fread(buffer + buffer_tail, 1, BUFFERSIZE - buffer_tail, input);
			if (length == 0)
				eof = 1;
			else
				buffer_tail += length;
		} while (eof == 0 && buffer_tail < BUFFERSIZE);
	}

	while (buffer_head < buffer_tail && 
			(buffer[buffer_head] == ' ' ||
			 buffer[buffer_head] == '\t' ||
 			 buffer[buffer_head] == '\r' ||
 			 buffer[buffer_head] == '\n'))
		buffer_head ++;

	if (buffer_head >= buffer_tail)
		return NULL;

	switch(buffer[buffer_head]) {
		case '[': buffer_head ++; return "[";
		case ']': buffer_head ++; return "]";
		case '{': buffer_head ++; return "{";
		case '}': buffer_head ++; return "}";
		case ':': buffer_head ++; return ":";
		case ',': buffer_head ++; return ",";
	}

	if (buffer[buffer_head] == '"') {
		int ptr;
		for (ptr = buffer_head + 1; ptr < buffer_tail && buffer[ptr] != '"'; ptr ++) {
			if (buffer[ptr] == '\\')
				ptr ++;
		}
		if (ptr >= buffer_tail) {
			fprintf(stderr, "unfinished string\n");
			exit(1);
		}
		retval = mystrndup(buffer + buffer_head + 1, ptr - buffer_head - 1);
		buffer_head = ptr + 1;
	} else if (buffer[buffer_head] == '-' || buffer[buffer_head] == '+' ||
			(buffer[buffer_head] >= '0' && buffer[buffer_head] <= '9')) {
		int ptr;
		for (ptr = buffer_head + 1; ptr < buffer_tail &&
				((buffer[ptr] >= '0' && buffer[ptr] <= '9') ||
				 buffer[ptr] == '.' ||
				 buffer[ptr] == 'e' ||
				 buffer[ptr] == 'E'); ptr ++)
			;
		retval = mystrndup(buffer + buffer_head, ptr - buffer_head);
		buffer_head = ptr;
	} else if ((buffer[buffer_head] >= 'a' && buffer[buffer_head] <= 'z') ||
			(buffer[buffer_head] >= 'A' && buffer[buffer_head] <= 'Z') ||
			buffer[buffer_head] == '_') {
		int ptr;
		for (ptr = buffer_head + 1; ptr < buffer_tail &&
				((buffer[ptr] >= 'a' && buffer[ptr] <= 'z') ||
				 (buffer[ptr] >= 'A' && buffer[ptr] <= 'Z') ||
				 buffer[ptr] == '_'); ptr ++)
			;
		retval = mystrndup(buffer + buffer_head, ptr - buffer_head);
		buffer_head = ptr;
	} else {
		fprintf(stderr, "unexpected character %c\n", buffer[buffer_head]);
		exit(1);
	}

	return retval;
}

static int process (void)
{
	const char *token;
	while ((token = next_token()) != NULL) {
		printf("token: \"%s\"\n", token);
	}
	return 0;
}

static void help (void)
{
	printf("help\n");
}

int main (int argc, char *argv[])
{
	int opt;
	const char *infile = NULL, *outfile = NULL;

	while ((opt = getopt(argc, argv, "i:o:p:c:a:b:h")) != -1) {
		switch (opt) {
			case 'i': infile = optarg; break;
			case 'o': outfile = optarg; break;
			case 'p': path_del = optarg; break;
			case 'c': column_del = optarg; break;
			case 'a': array_prefix = optarg; break;
			case 'b': object_prefix = optarg; break;
			case 'h': help(); return 0;
			default: fprintf(stderr, "Unknown option. check %s -h.\n", argv[0]); return 1;
		}
	}

	if (optind < argc) {
		fprintf(stderr, "unknown argument %s. input file should be -i <file>.\n", argv[optind]);
		return 1;
	}

	input = infile ? fopen(infile, "r") : stdin;
	if (input == NULL) {
		fprintf(stderr, "failed to open %s\n", infile);
		return 1;
	}
	output = outfile ? fopen(outfile, "w") : stdout;
	if (output == NULL) {
		fprintf(stderr, "failed to open %s\n", outfile);
		return 1;
	}

	return process();
}
