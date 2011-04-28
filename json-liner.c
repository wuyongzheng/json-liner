#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

#define TOKENSIZE 8192
#define PATHSIZE 65536

static char *path_del = "/";
static char *column_del = "\t";
static char *array_prefix = "@";
static char *object_prefix = "%";
static int array_base = 0;
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
	static char buffer[TOKENSIZE];
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
			if (i >= TOKENSIZE - 6) {
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
			if (i >= TOKENSIZE - 2) {
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
			if (i >= TOKENSIZE - 2) {
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
		for (i = array_base; ; i++) {
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
			if (new_length >= size - 2)
				return 1;
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
	char prefix[PATHSIZE];
	const char *token;

	prefix[0] = '\0';
	retval = process_node(prefix, 0, PATHSIZE);
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
	puts("json-liner: grep friendly JSON dumper");
	puts("Usage: json-liner [-i infile] [-o outfile] [-p pathdel] [-c columndel]");
	puts("                  [-a arrprefix] [-b objprefix] [-0 arraybase] [-h]");
	puts("  -i infile    : input file path. Default is stdin.");
	puts("  -o outfile   : output file path. Default is stdout.");
	puts("  -p pathdel   : path delimiter. Default is /.");
	puts("  -c columndel : column delimiter. Default is TAB.");
	puts("                 Tips: pipe to \"cut -f 2\" to extract value.");
	puts("  -a arrprefix : array prefix. Default is @.");
	puts("  -b objprefix : object prefix. Default is %.");
	puts("  -0 arraybase : array base. Default is 0.");
	puts("                 Tips: set to 1000 so that indexes are (almost) fixed size.");
	puts("  -h           : show this help message.");
	puts("Example:");
	puts("  $ echo 123 | json-liner");
	puts("  /\t123");
	puts("  $ echo '{\"a\":{\"b\":1,\"c\":2},\"d\":[\"x\",1]}' | json-liner");
	puts("  /%a/%b\t1");
	puts("  /%a/%c\t2");
	puts("  /%d/@0\tx");
	puts("  /%d/@1\t1");
}

int main (int argc, char *argv[])
{
	int opt, retval;
	const char *infile = NULL, *outfile = NULL;

	while ((opt = getopt(argc, argv, "i:o:p:c:a:b:0:h")) != -1) {
		switch (opt) {
			case 'i': infile = optarg; break;
			case 'o': outfile = optarg; break;
			case 'p': path_del = optarg; break;
			case 'c': column_del = optarg; break;
			case 'a': array_prefix = optarg; break;
			case 'b': object_prefix = optarg; break;
			case '0': array_base = atoi(optarg) ; break;
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

	retval = process();

	if (infile)
		fclose(input);
	if (outfile)
		fclose(output);

	return retval;
}
