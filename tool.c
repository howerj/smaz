#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include "smaz.h"

static int smaz_tests(FILE *report) {
	assert(report);
	char in[512] = { 0, };
	char out[4096] = { 0, };
	char d[4096] = { 0, };
	static const char *strings[] = {
		"This is a small string",
		"foobar",
		"the end",
		"not-a-g00d-Exampl333",
		"Smaz is a simple compression library",
		"Nothing is more difficult, and therefore more precious, than to be able to decide",
		"this is an example of what works very well with smaz",
		"1000 numbers 2000 will 10 20 30 compress very little",
		"and now a few italian sentences:",
		"Nel mezzo del cammin di nostra vita, mi ritrovai in una selva oscura",
		"Mi illumino di immenso",
		"L'autore di questa libreria vive in Sicilia",
		"try it against urls",
		"http://google.com",
		"http://programming.reddit.com",
		"http://github.com/antirez/smaz/tree/master",
		"/media/hdb1/music/Alben/The Bla",
		NULL
	};

	for (int j = 0; strings[j];) {
		int comprlen = smaz_compress(strings[j], strlen(strings[j]), out, sizeof(out));
		int comprlevel = 100 - (( 100 * comprlen) / strlen(strings[j]));
		int decomprlen = smaz_decompress(out,comprlen,d,sizeof(d));
		if (strlen(strings[j]) != (unsigned)decomprlen || memcmp(strings[j], d, decomprlen)) {
			if (fprintf(report, "BUG: error compressing '%s'\n", strings[j]) < 0) return -1;
			return -1;
		}
		if (comprlevel < 0) {
			if (fprintf(report, "'%s' enlarged by %d%%\n", strings[j], -comprlevel) < 0) return -1;
		} else {
			if (fprintf(report, "'%s' compressed by %d%%\n", strings[j], comprlevel) < 0) return -1;
		}
		j++;
	}

	int times = 1000000;
	if (fprintf(report, "Compressing and decompressing %d test strings...\n", times) < 0) return -1;
	while (times--) {
		char charset[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvxyz/. ";
		int ranlen = rand() % 512;

		for (int j = 0; j < ranlen; j++) {
			if (times & 1)
				in[j] = charset[rand() % (sizeof(charset) - 1)];
			else
				in[j] = (char)(rand() & 0xff);
		}
		int comprlen = smaz_compress(in, ranlen, out, sizeof(out));
		int decomprlen = smaz_decompress(out, comprlen, d, sizeof(out));

		if (ranlen != decomprlen || memcmp(in ,d ,ranlen)) {
			if (fprintf(report, "Bug! TEST NOT PASSED\n") < 0) return -1;
			return -1;
		}
		/* if (fprintf(report, "%d -> %d\n", comprlen, decomprlen) < 0) return -1; */
	}
	if (fprintf(report, "TEST PASSED :)\n") < 0) return -1;
	return 0;
}

static int usage(FILE *out, const char *arg0) {
	assert(out);
	assert(arg0);
	if (fprintf(out, "usage: %s options infile outfile\n", arg0) < 0) return -1; 
	if (fprintf(out, "Options: \n") < 0) return -1;
	if (fprintf(out, "\t-c compress\n") < 0) return -1;
	if (fprintf(out, "\t-d decompress\n") < 0) return -1;
	if (fprintf(out, "\t-t run tests\n") < 0) return -1;
	if (fprintf(out, "\t-h show this page\n") < 0) return -1;
	if (fprintf(out, "\t--help same as -h\n") < 0) return -1;
	return 0;
}

static FILE *fopen_or_die(const char *name, const char *mode) {
	assert(name);
	assert(mode);
	const int e = errno;
	errno = 0;
	FILE *r = fopen(name, mode);
	if (!r) {
		(void)fprintf(stderr, "unable to open file '%s' in mode '%s': %s\n", name, mode, strerror(errno));
		exit(EXIT_FAILURE);
	}
	errno = e;
	return r;
}

static void *calloc_or_die(size_t sz) {
	void *r = calloc(sz, 1);
	if (!r) {
		(void)fprintf(stderr, "unable to calloc array of size: %ld\n", (long)sz);
		exit(EXIT_FAILURE);
	}
	return r;
}

typedef struct {
	unsigned char *b;
	size_t len;
} buffer_t;

static int slurp(FILE *in, buffer_t *obuf) { /* might want slurp until newline as well */
	assert(in);
	assert(obuf);
	buffer_t r = { NULL, 0, };
	for (size_t pos = 0;!feof(in) && !ferror(in);) {
		const size_t chunk = 256;
		assert((r.len + chunk) > r.len);
		unsigned char *n = realloc(r.b, r.len + chunk);
		if (!n) {
			free(r.b);
			return -1;
		}
		r.b = n;
		const size_t sz = fread(r.b + pos, 1, chunk, in);
		r.len += sz;
		if (sz != chunk) {
			break;
		}
		pos += sz;
	}
	if (ferror(in)) {
		free(r.b);
		return -1;
	}
	*obuf = r;
	return 0;
}

int main(int argc, char const **argv) { 
	int r = 0, compress = 0;
	if (argc < 2) {
		usage(stderr, argv[0]);
		return 1;
	} else if (!strcmp(argv[1], "-c")) {
		if (argc < 4) {
			usage(stderr, argv[0]);
			return 1;
		}
		compress = 1;
	} else if (!strcmp(argv[1], "-d")) {
		if (argc < 4) {
			usage(stderr, argv[0]);
			return 1;
		}
		compress = 0;
	} else if (!strcmp(argv[1], "-t")) {
		return smaz_tests(stdout);
	} else if (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help")) {
		usage(stderr, argv[0]);
		return 0;
	} else {
		(void)fprintf(stderr, "invalid option: %s\n", argv[1]);
		return 1;
	}

	FILE *fp_in = fopen_or_die(argv[2], "rb");
	FILE *fp_out = fopen_or_die(argv[3], "wb");

	buffer_t inb = { NULL, 0, };
	if (slurp(fp_in, &inb) < 0) {
		(void)fprintf(stderr, "failed to slurp file\n");
		return 1;
	}

	long output_size = 0;
	size_t file_size = inb.len;
	size_t max_output_size = file_size * (compress ? 2 : 8); /* TODO: Max expansion calc */
	assert(max_output_size >= file_size);

	char *in = (char*)inb.b;
	char *out = calloc_or_die(max_output_size); 

	if (compress) {
		output_size = smaz_compress(in, file_size, out, max_output_size);
	} else {
		output_size = smaz_decompress(in, file_size, out, max_output_size);
	}

	if (output_size != fwrite(out, 1, output_size, fp_out)) {
		(void)fprintf(stderr, "file write of size %ld failed\n", output_size);
		r = -1;
	}
	if (fclose(fp_in) < 0)
		r = -1;
	if (fclose(fp_out) < 0)
		r = -1;
	free(in);
	free(out);
	return r ? 1 : 0;
}

