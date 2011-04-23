#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

static char *path_del = "/";
static char *column_del = "\t";
static char *array_prefix = "@";
static char *object_prefix = "%";
static FILE *input, *output;

static int process (void)
{
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
