#ifndef NARG_H
#define NARG_H
#include <stdint.h>
#include <stdio.h>
#ifndef __cplusplus
# include <stdbool.h>
#endif

/*
 * Narg takes multiples of exactly N arguments.
 */

struct narg_optspec {
	const char *shortopt; // UTF-8
	const char *longopt;  // UTF-8
	const char *metavar;  // UTF-8
	const char *help;     // UTF-8
};

struct _narg_special_values_for_metavar {
	char ignore_rest; // '\0'
};
extern const struct _narg_special_values_for_metavar narg_metavar;

struct narg_paramret {
	unsigned paramc; // param count
	const char **paramv; // can be initialized to NULL, or an array of string literals (const char*) to signify a default value. When parameters are found, this will point into argv, replacing any default value, and argv is reordered to make multiple params contiguous.
};

struct narg_result {
	enum {
		NARG_ENOSUCHOPTION = 1,
		NARG_EMISSINGPARAM,
		NARG_EUNEXPECTEDPARAM,
		NARG_EILSEQ
	} err;
	unsigned arg;
};

#ifdef __cplusplus
# define DEFAULTVALUE(x) =x
#else
# define DEFAULTVALUE(x)
#endif

#ifdef __cplusplus
extern "C" {
#endif

struct narg_result
narg_argparse(
	char **argv,
	struct narg_paramret *retv,
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

void
narg_printopt_unlocked(
	FILE *fp,
	const struct narg_optspec *optv,
	const struct narg_paramret *retv,
	unsigned optc,
	unsigned dashes_longopt,
	unsigned width
);

int_fast8_t
narg_utf8len(const char *first);

#undef DEFAULTVALUE

#ifdef __cplusplus
} //extern C
#endif

#endif //NARG_H
