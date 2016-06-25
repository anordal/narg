#ifndef NARG_H
#define NARG_H
#include <stdint.h>
#include <stdio.h>
#ifndef __cplusplus
#	include <stdbool.h>
#endif

/*
 * Narg takes exactly N arguments.
 */

struct narg_optspec {
	const char* shortopt; // UTF-8
	const char* longopt;  // UTF-8
	const char* metavar;  // UTF-8
	const char* help;     // UTF-8
};

struct _narg_special_values_for_metavar {
	char ignore_rest; // '\0'
};
extern const struct _narg_special_values_for_metavar narg_metavar;

struct narg_result {
	enum {
		NARG_ENOSUCHOPTION = 1,
		NARG_EMISSINGPARAM,
		NARG_EUNEXPECTEDPARAM
	} err;
	unsigned arg;
};

#ifdef __cplusplus
#define DEFAULTVALUE(x) =x
#else
#define DEFAULTVALUE(x)
#endif

#ifdef __cplusplus
extern "C" {
#endif

struct narg_result
narg_argparse(
	const char ***ansv,
	char **argv,
	const struct narg_optspec *optv,
	unsigned optc,
	unsigned dashes_longopt DEFAULTVALUE(2),
	unsigned max_positional_args DEFAULTVALUE(~0)
);

unsigned
narg_terminalwidth(FILE *fp);

void
narg_indentputs_unlocked(
	FILE *fp, unsigned *posPtr,
	unsigned indent, unsigned width, const char *str
);

unsigned
narg_wordcount(const char *);

void
narg_printopt_unlocked(
	FILE *fp,
	const char ***ansv,
	const struct narg_optspec *optv,
	unsigned optc,
	unsigned dashes_longopt,
	unsigned width
);

#undef DEFAULTVALUE

#ifdef __cplusplus
} //extern C
#endif

#endif
