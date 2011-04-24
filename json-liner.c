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

static inline int getc_err (void)
{
	int c = getc(input);
	if (c == EOF) {
		fprintf(stderr, "lexer: unexpected EOF\n");
		exit(1);
	}
	return c;
}

static const char *get_token (void)
{
	static char buffer[BUFFERSIZE];
	int c;

	do {
		c = getc(input);
		if (c == EOF)
			return NULL;
	} while (c == ' ' || c == '\t' || c == '\r' || c == '\n');

	switch(c) {
		case '[': return "[";
		case ']': return "]";
		case '{': return "{";
		case '}': return "}";
		case ':': return ":";
		case ',': return ",";
	}

	if (c == '"') {
		int i = 0;
		while (1) {
			c = getc_err();
			if (c == '"')
				break;
			buffer[i ++] = c;
			if (c == '\\') {
				c = getc_err();
				buffer[i ++] = c;
				if (c == 'u') {
					buffer[i ++] = getc_err();
					buffer[i ++] = getc_err();
					buffer[i ++] = getc_err();
					buffer[i ++] = getc_err();
				}
			}
			if (i >= BUFFERSIZE - 6) {
				fprintf(stderr, "lexer: string too long\n");
				exit(1);
			}
		}
		buffer[i] = '\0';
	} else if (c == '-' || c == '+' || (c >= '0' && c <= '9')) {
		int i = 0;
		buffer[i ++] = c;
		while (1) {
			c = getc(input);
			if (c == EOF)
				break;
			if ((c < '0' || c > '9') && c != '.' && c != 'e' && c != 'E') {
				assert(c == ungetc(c, input));
				break;
			}
			if (i >= BUFFERSIZE - 2) {
				fprintf(stderr, "lexer: number too long\n");
				exit(1);
			}
			buffer[i ++] = c;
		}
		buffer[i] = '\0';
	} else if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_') {
		int i = 0;
		buffer[i ++] = c;
		while (1) {
			c = getc(input);
			if (c == EOF)
				break;
			if ((c < 'a' || c > 'z') && (c < 'A' || c > 'Z') && c != '_') {
				assert(c == ungetc(c, input));
				break;
			}
			if (i >= BUFFERSIZE - 2) {
				fprintf(stderr, "lexer: constant too long\n");
				exit(1);
			}
			buffer[i ++] = c;
		}
		buffer[i] = '\0';
	} else {
		fprintf(stderr, "lexer: unexpected character %c\n", c);
		exit(1);
	}

	return buffer;
}

static int process_node (char *prefix, int length, int size)
{
	const char *token = get_token();
	int retval;

	if (token == NULL) {
		fprintf(stderr, "unexpected EOF\n");
		return 1;
	}

	if (token[0] == '[') {
		int i;
		for (i = 0; ; i++) {
			int new_length = length + snprintf(prefix + length, size - length - 1, "%s%s%d", path_del, array_prefix, i);
			if (new_length >= size - 2)
				return 1;
			if ((retval = process_node(prefix, new_length, size)))
				return retval;
			if ((token = get_token()) == NULL) {
				fprintf(stderr, "unexpected EOF in array. expect ','\n");
				return 1;
			}
			if (strcmp(token, "]") == 0)
				return 0;
			if (strcmp(token, ",") != 0) {
				fprintf(stderr, "unexpected token %s in array\n", token);
				return 1;
			}
		}
	} else if (token[0] == '{') {
		while (1) {
			int new_length;
			if ((token = get_token()) == NULL) {
				fprintf(stderr, "unexpected EOF at object name\n");
				return 1;
			}
			new_length = length + snprintf(prefix + length, size - length - 1, "%s%s%s", path_del, object_prefix, token);
			if ((token = get_token()) == NULL) {
				fprintf(stderr, "unexpected EOF in object. expect ':'\n");
				return 1;
			}
			if (strcmp(token, ":") != 0) {
				fprintf(stderr, "expected ':' but got '%s'\n", token);
				return 1;
			}
			if ((retval = process_node(prefix, new_length, size)))
				return retval;
			if ((token = get_token()) == NULL) {
				fprintf(stderr, "unexpected EOF in object. expect ','\n");
				return 1;
			}
			if (strcmp(token, "}") == 0)
				return 0;
			if (strcmp(token, ",") != 0) {
				fprintf(stderr, "unexpected token %s in object\n", token);
				return 1;
			}
		}
	} else {
		if (length == 0)
			fprintf(output, "%s%s%s\n", path_del, column_del, token);
		else
			fprintf(output, "%s%s%s\n", prefix, column_del, token);
		return 0;
	}
}

static int process (void)
{
	int retval;
	char prefix[BUFFERSIZE];
	const char *token;

	prefix[0] = '\0';
	retval = process_node(prefix, 0, BUFFERSIZE);
	if (retval)
		return retval;

	if ((token = get_token()) != NULL) {
		fprintf(stderr, "redundant input %s ...\n", token);
		return 1;
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
