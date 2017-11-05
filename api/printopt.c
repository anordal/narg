// This Source Code Form is subject to the terms
// of the Mozilla Public License, v. 2.0.
// If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef TEST
#define _DEFAULT_SOURCE //unlocked stdio, fileno
#include "narg.h"

#ifdef __linux__
# include <sys/ioctl.h>
# include <unistd.h>
#endif
#include <string.h> //strlen
#include <ctype.h>  //isspace
unsigned narg_terminalwidth(FILE *fp) {
	unsigned width = 80;
#ifdef __linux__
	int fd = fileno(fp);
	struct winsize w;
	if (0 == ioctl(fd, TIOCGWINSZ, &w)) width = w.ws_col;
#endif
	return width;
}

typedef struct {
	unsigned x1, x2;
} pair_t;

static pair_t findbreakpoint(const char *line, unsigned width) {
	pair_t pos={0, 0}, breakpoint={~0, ~0};
	for (;;) {
		const int charlen = narg_utf8len(line + pos.x1);
		if (charlen <= 0 || line[pos.x1] == '\n') {
			return pos;
		} else if (line[pos.x1] == ' ') {
			breakpoint = pos;
		}
		if (pos.x2 == width) {
			break;
		}
		pos.x1 += charlen;
		pos.x2 += 1;
	}
	return (breakpoint.x2 != ~0u) ? breakpoint : pos;
}

void narg_indentputs_unlocked(
	FILE *fp, unsigned *posPtr,
	unsigned indent, unsigned width, const char *str
){
	unsigned pos = *posPtr;
	for (;;) { //line by line
		for (; pos < indent; pos++) {
			putc_unlocked(' ', fp);
		}
		pair_t breakpoint = findbreakpoint(str, width - pos);
		fwrite_unlocked(str, 1, breakpoint.x1, fp);
		str += breakpoint.x1;
		pos += breakpoint.x2;
		if (*str == '\0') {
			break;
		}
		putc_unlocked('\n', fp);
		str++;
		pos = 0;
	}
	*posPtr = pos;
}

static pair_t utf8strlen(const char *str) {
	pair_t ret = {0, 0};
	int charlen;
	while ((charlen=narg_utf8len(str + ret.x1)) > 0) {
		ret.x1 += charlen;
		ret.x2 += 1;
	}
	return ret;
}

static pair_t nullutf8strlen(const char *s) {
	if(s) return utf8strlen(s);
	pair_t niks = {0, 0};
	return niks;
}

static pair_t utf8strcpy(char *restrict dst, const char *restrict src) {
	pair_t ret = {0, 0};
	for (unsigned i=0; ; i++) {
		if (ret.x1 == i) {
			int charlen = narg_utf8len(src + i);
			if (charlen <= 0) {
				break;
			}
			ret.x1 += charlen;
			ret.x2 += 1;
		}
		dst[i] = src[i];
	}
	return ret;
}

static pair_t pairmax(pair_t a, pair_t b) {
	if(a.x1 < b.x1) a.x1 = b.x1;
	if(a.x2 < b.x2) a.x2 = b.x2;
	return a;
}

static pair_t pairaddscalar(pair_t a, unsigned b) {
	a.x1 += b;
	a.x2 += b;
	return a;
}

static pair_t pairaddpair(pair_t a, pair_t b) {
	a.x1 += b.x1;
	a.x2 += b.x2;
	return a;
}

void narg_printopt_unlocked(
	FILE *fp,
	unsigned width,
	const struct narg_optspec *optv,
	const struct narg_optparam *ansv,
	unsigned optc,
	narg_dgettext_lookalike_f i18n_translator,
	const char *i18n_domain,
	unsigned dashes_longopt
){
	const unsigned dashes_shortopt = (dashes_longopt > 1)
		? dashes_longopt - 1
		: dashes_longopt
	;

	pair_t shortlen={0, 0}, longlen={0, 0};
	unsigned o;
	for (o=0; o < optc; o++) {
		shortlen = pairmax(shortlen, nullutf8strlen(optv[o].shortopt));
		longlen = pairmax(longlen, pairaddpair(nullutf8strlen(optv[o].longopt), nullutf8strlen(optv[o].metavar)));
	}
	if (shortlen.x2) shortlen = pairaddscalar(shortlen, dashes_shortopt);
	if (shortlen.x2 && longlen.x2) shortlen = pairaddscalar(shortlen, 1);
	if (longlen.x2) longlen = pairaddscalar(longlen, dashes_longopt);
	longlen = pairaddscalar(longlen, 2); //2 spaces before help

	pair_t prelen = pairaddpair(shortlen, longlen);
	unsigned indent = prelen.x2 % width;
	char preprint[prelen.x1]; //unterminated
	
	for (o=0; o < optc; o++) {
		pair_t wpos = {0, 0};

		if (optv[o].shortopt) {
			memset(preprint + wpos.x1, '-', dashes_shortopt);
			wpos = pairaddscalar(wpos, dashes_shortopt);
			wpos = pairaddpair(wpos, utf8strcpy(preprint + wpos.x1, optv[o].shortopt));
		}
		memset(preprint + wpos.x1, ' ', shortlen.x2 - wpos.x2);
		wpos = pairaddscalar(wpos, shortlen.x2 - wpos.x2);

		if (optv[o].longopt) {
			memset(preprint + wpos.x1, '-', dashes_longopt);
			wpos = pairaddscalar(wpos, dashes_longopt);
			wpos = pairaddpair(wpos, utf8strcpy(preprint + wpos.x1, optv[o].longopt));
		} else {
			memset(preprint + wpos.x1, ' ', dashes_longopt);
			wpos = pairaddscalar(wpos, dashes_longopt);
		}
		if (optv[o].metavar) {
			wpos = pairaddpair(wpos, utf8strcpy(preprint + wpos.x1, optv[o].metavar));
		}
		memset(preprint + wpos.x1, ' ', prelen.x2 - wpos.x2);
		wpos = pairaddscalar(wpos, prelen.x2 - wpos.x2);

		fwrite_unlocked(preprint, 1, wpos.x1, fp);
		wpos.x2 %= width;
		
		if (optv[o].help) {
			const char *help_translated = i18n_translator(i18n_domain, optv[o].help);
			narg_indentputs_unlocked(fp, &wpos.x2, indent, width, help_translated);
		}
		if (ansv[o].paramc) {
			char buf[100];
			char *const end = buf + sizeof(buf);
			char *pos = stpcpy(buf, " [");
			for (unsigned a=0; ;) {
				pos = stpncpy(pos, ansv[o].paramv[a++], end-pos);
				if (a == ansv[o].paramc || end-pos < 2) {
					break;
				}
				*pos++ = ' ';
			}
			if (end-pos < 2) {
				pos = stpcpy(end-2-strlen("…"), "…");
			}
			stpcpy(pos, "]");
			narg_indentputs_unlocked(fp, &wpos.x2, indent, width, buf);
		}
		putc_unlocked('\n', fp);
	}
}

#else //TEST
#undef TEST
#define _POSIX_C_SOURCE 200112L //popen
#include "narg.h"
#include "../testapi/testability.h"
#include <stdlib.h> //getenv, setenv

static void expect_fp(int *status, FILE *fp, const char *expected) {
	const char *s = expected;
	char fc, sc;
	do {
		int c = fgetc(fp);
		fc = (c != EOF) ? c : '\0';
		sc = *s++;
	} while (fc == sc && fc != '\0');
	if (fc != '\0') {
		fwrite(expected, 1, s-expected-1, stderr);
		fprintf(stderr, "\nExpected byte: %02x\nActual   byte: %02x\n", sc, fc);
		fputs(s, stderr);
		*status = 1;
	}
}

static char *fake_dgettext(const char *unused, const char *s) {
	(void)unused; // please the compiler
	return (char*)s;
}

int main(int argc, char *argv[]) {
	static const char env_doit[] = "NARG_TEST_CHILD_DOIT";
	static const int EXIT_NOT_THE_RETURN_VALUE_YOU_ARE_LOOKING_FOR = 2;
	if (argc != 1 || getenv(env_doit)) {
		const struct narg_optspec optv[] = {
			{"h","help",NULL,"Show help text"},
			{"ø̧̇","one","=ARG","Option that takes 1 parameter"},
			{"t","two"," ARG1 ARG2","Option that takes 2 parameters"},
			{NULL,"just-a-very-long-option"," ARG1 ARG2 ARG3 ARG4","Oh well"},
			{NULL,"",&narg_metavar.ignore_rest,"Don\'t interpret further arguments as options"}
		};
		struct narg_optparam ansv[ARRAY_SIZE(optv)] = {
			{0, NULL},
			{3, (const char*[]){"these","by","default"}},
			{2, (const char*[]){"default","values"}},
			{0, NULL},
			{0, NULL}
		};
		unsigned width = narg_terminalwidth(stdout);
		narg_printopt_unlocked(stdout, width, optv, ansv, ARRAY_SIZE(optv), fake_dgettext, NULL, 2);
		return EXIT_NOT_THE_RETURN_VALUE_YOU_ARE_LOOKING_FOR;
	} else {
		int status = 0;

		if (0 != setenv(env_doit, "", 0)) {
			perror("setenv");
			return EXIT_NOT_THE_RETURN_VALUE_YOU_ARE_LOOKING_FOR;
		}
		FILE *self_rd_fp = popen(argv[0], "r");
		if (self_rd_fp == NULL) {
			perror("popen");
			return EXIT_NOT_THE_RETURN_VALUE_YOU_ARE_LOOKING_FOR;
		}
		static const char expect[] =
			"-h --help                                         Show help text\n"
			"-ø̧̇ --one=ARG                                      Option that takes 1 parameter\n"
			"                                                  [these by default]\n"
			"-t --two ARG1 ARG2                                Option that takes 2 parameters\n"
			"                                                  [default values]\n"
			"   --just-a-very-long-option ARG1 ARG2 ARG3 ARG4  Oh well\n"
			"   --                                             Don\'t interpret further\n"
			"                                                  arguments as options\n"
		;
		expect_fp(&status, self_rd_fp, expect);
		pclose(self_rd_fp);
		return status;
	}
}

#endif //TEST
