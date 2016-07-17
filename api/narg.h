#ifndef NARG_H
#define NARG_H
#include <stdint.h>
#include <stdio.h>
#ifndef __cplusplus
# include <stdbool.h>
#endif

struct narg_optspec {
	const char *shortopt; // UTF-8
	const char *longopt;  // UTF-8
	const char *metavar;  // UTF-8
	const char *help;     // gettext_noop or equivalent
};

struct narg_optparam {
	unsigned paramc;     // number of parameters in paramv considered part of this option's parameters
	const char **paramv; // can point to either NULL, an array of default values, or an offset of argv
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

struct _narg_special_values_for_metavar {
	char ignore_rest; // '\0'
};
extern const struct _narg_special_values_for_metavar narg_metavar;

typedef char*(*narg_dgettext_lookalike_f)(const char*, const char*);

#ifdef __cplusplus
# define DEFAULTVALUE(x) =x
#else
# define DEFAULTVALUE(x)
#endif

#ifdef __cplusplus
extern "C" {
#endif

struct narg_result narg_findopt(
	char **argv,
	const struct narg_optspec *optv,
	struct narg_optparam *ansv,
	unsigned optc,
	unsigned max_positional_args DEFAULTVALUE(~0),
	unsigned dashes_longopt DEFAULTVALUE(2)
);

unsigned narg_terminalwidth(FILE *fp);

void narg_indentputs_unlocked(
	FILE *fp, unsigned *posPtr,
	unsigned indent, unsigned width, const char *str
);

void narg_printopt_unlocked(
	FILE *fp,
	unsigned width,
	const struct narg_optspec *optv,
	const struct narg_optparam *ansv,
	unsigned optc,
	narg_dgettext_lookalike_f i18n_translator,
	const char *i18n_domain DEFAULTVALUE(NULL),
	unsigned dashes_longopt DEFAULTVALUE(2)
);

int_fast8_t narg_utf8len(const char *first);

#undef DEFAULTVALUE

#ifdef __cplusplus
} //extern C
#endif

#endif //NARG_H
