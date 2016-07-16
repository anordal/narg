#include "narg.h"

#ifndef TEST
#include <stddef.h>

#include "../inc/str.c"
#include "../inc/wordcount.c"
#include "../inc/reverse_slices.c"
#include "../inc/assign_params.c"

const struct _narg_special_values_for_metavar narg_metavar = {0};

struct narg_result
narg_argparse(
	char **argv,
	struct narg_paramret *retv,
	const struct narg_optspec *optv,
	unsigned optc,
	unsigned dashes_longopt,
	unsigned max_positional_args)
{
	// Be able to guarrantee that the character data are not modified
	// even though the api does not; that would require a client side cast.
	const char **args = (const char**)argv;
	argv = NULL;

	const unsigned dashes_shortopt = (dashes_longopt > 1)
		? dashes_longopt - 1
		: dashes_longopt
	;

	struct narg_paramret posargs = {0}; // positional (non-optional) arguments
	for (unsigned a=1; ; ++a) {
		const char *arg = args[a];

		if (arg == NULL) {
ignore_rest:
			max_positional_args = 0;
positional_argument:
			assign_params(&posargs, args+a, 1, 0, &posargs, retv, optc, args);
			if (max_positional_args) {
				--max_positional_args;
				continue;
			}
			break;
		}

		const unsigned dashcount = leadingchars(arg, '-');
		if (dashcount < dashes_shortopt) {
			goto positional_argument;
		}
		arg += dashcount;

		//shortopt loop
		for (;;) {
			unsigned len_paramsep;
			unsigned expectedlen;
			if (dashcount >= dashes_longopt) {
				len_paramsep = 1; // strlen("=")
				expectedlen = strlen_delim(arg, '=');
			} else {
				len_paramsep = 0; // strlen("")
				int_fast8_t test = narg_utf8len(arg);
				if (test == -1) {
					struct narg_result ret = { .err=NARG_EILSEQ, .arg=a };
					return ret;
				}
				expectedlen = test;
			}
			unsigned o;
			for (o=0; o != optc; o++) {
				if (dashcount >= dashes_longopt) {
					if (has_prefix_of_len(arg, optv[o].longopt, expectedlen)) {
						break;
					}
				}
				if (dashcount == dashes_shortopt) {
					if (has_prefix_of_len(arg, optv[o].shortopt, expectedlen)) {
						break;
					}
				}
			}
			if (o == optc) {
				struct narg_result ret = { .err=NARG_ENOSUCHOPTION, .arg=a };
				return ret;
			}
			arg += expectedlen;

			const unsigned expectedparams = narg_wordcount(optv[o].metavar);
			if (expectedparams == 0) {
				retv[o].paramv = args + a;
				retv[o].paramc = 1;
				if (arg[0]) {
					if (len_paramsep) {
						struct narg_result ret = { .err = NARG_EUNEXPECTEDPARAM, .arg = a };
						return ret;
					}
					continue; //shortopt loop
				}
				if (optv[o].metavar == &narg_metavar.ignore_rest){
					arg = args[++a];
					goto ignore_rest;
				}
			} else {
				const unsigned num_firstparam = (arg[0])
					? 1 // the first parameter is in the option argument
					: 0 // zero parameters in the option argument
				;
				const char **paramvec = args + a + 1 - num_firstparam;
				unsigned paramcount = num_firstparam;
				while (paramcount != expectedparams && paramvec[paramcount]) {
					++paramcount;
				}
				if (paramcount != expectedparams) {
					struct narg_result ret = { .err = NARG_EMISSINGPARAM, .arg = a };
					return ret;
				}
				if (num_firstparam) {
					paramvec[0] = arg + len_paramsep;
				}
				assign_params(retv+o, paramvec, expectedparams, 1-num_firstparam, &posargs, retv, optc, args);
				a += expectedparams - num_firstparam;
			}
			break;
		}
	}

	struct narg_result ret = { .err=0, .arg=posargs.paramv-args };
	return ret;
}

#else //TEST
#undef TEST
#include "../testapi/testability.h"
#include <string.h>

static void ignore_rest(int *status) {
	const struct narg_optspec optv[] = {
		{"h","help",NULL,"Show help text"},
		{"o","output","OUTFILE","Set output file"},
		{NULL,"",&narg_metavar.ignore_rest,"Treat subsequent arguments as positional"}
	};
	struct narg_paramret retv[ARRAY_SIZE(optv)] = {0};
	const char *argv[] = {
		NULL,
		"-o",
		"--help",
		"pos1",
		"--",
		"--help",
		NULL
	};
	struct narg_result res = narg_argparse((char**)argv, retv, optv, ARRAY_SIZE(optv), 2, ~0);
	expect_i(status, res.err, 0);
	expect_i(status, res.arg, 4);
	expect_paramret(status, retv+0, 0, (const char *[]){0});
	expect_paramret(status, retv+1, 1, (const char *[]){"--help"});
	expect_paramret(status, retv+2, 1, (const char *[]){"--"});
	compare_slices(status, argv+res.arg, (const char*[]){"pos1","--help"}, 2);
}

static void max_positional(int *status){
	const struct narg_optspec optv[] = {
		{"a",NULL,"PARAM",""},
		{"b",NULL,NULL,""},
		{"c",NULL,NULL,""}
	};
	struct narg_paramret retv[ARRAY_SIZE(optv)] = {0};
	const char *argv[] = {
		NULL,
		"-a",
		"p",
		"A",
		"-b",
		"-ap2",
		"B", // stop here
		"-c", //canary
		NULL
	};
	struct narg_result res = narg_argparse((char**)argv, retv, optv, ARRAY_SIZE(optv), 2, 1);
	expect_i(status, res.err, 0);
	expect_i(status, res.arg, 5);
	expect_paramret(status, retv+0, 2, (const char *[]){"p","p2"});
	expect_paramret(status, retv+1, 1, (const char *[]){"-b"});
	expect_paramret(status, retv+2, 0, (const char *[]){0});
	compare_slices(status, argv+res.arg, (const char*[]){"A","B","-c"}, 3);
}

static void flagorgy(int *status){
	const struct narg_optspec optv[] = {
		{"æ","flag",NULL,""},
		{"ø̧̇","scalar"," VAL",""},
		{"å","pair"," VAL1 VAL2",""}
	};
	struct narg_paramret retv[ARRAY_SIZE(optv)] = {0};
	const char *argv[] = {
		NULL,
		"-æø̧̇å",
		NULL
	};
	struct narg_result res = narg_argparse((char**)argv, retv, optv, ARRAY_SIZE(optv), 2, ~0);
	expect_i(status, res.err, 0);
	expect_i(status, res.arg, 2);
	expect_paramret(status, retv+0, 1, (const char *[]){"å"});
	expect_paramret(status, retv+1, 1, (const char *[]){"å"});
	expect_paramret(status, retv+2, 0, (const char *[]){0});
}

static void dashes0(int *status){
	const struct narg_optspec optv[] = {
		{NULL,"help",NULL,""},
		{"se","search",NULL,""},
		{"in","install",NULL,""},
		{"if",NULL,"=IN",""},
		{NULL,"of","=OUT",""}
	};
	struct narg_paramret retv[ARRAY_SIZE(optv)] = {0};
	const char *argv[] = {
		NULL,
		"if=/dev/zero",
		"of=/dev/null",
		"---help", //ignore any amount of leading dashes
		"se",
		"install",
		"if", "Odd",
		"of", "Even",
		NULL
	};
	struct narg_result res = narg_argparse((char**)argv, retv, optv, ARRAY_SIZE(optv), 0, ~0);
	expect_i(status, res.err, 0);
	expect_i(status, res.arg, 10);
	expect_paramret(status, retv+0, 1, (const char *[]){"---help"});
	expect_paramret(status, retv+1, 1, (const char *[]){"se"});
	expect_paramret(status, retv+2, 1, (const char *[]){"install"});
	expect_paramret(status, retv+3, 2, (const char *[]){"/dev/zero", "Odd"});
	expect_paramret(status, retv+4, 2, (const char *[]){"/dev/null", "Even"});
}

static void fail(int *status) {
	const struct narg_optspec optv[] = {
		{"h","help",NULL,"Show help text"},
		{"o","output","OUTFILE","Set output file"},
		{NULL,"",&narg_metavar.ignore_rest,"Treat subsequent arguments as positional"}
	};
	struct narg_paramret retv[ARRAY_SIZE(optv)] = {0};
	struct narg_result res;

	const char *argv_enosuch[] = {
		NULL,
		"-h4",
		NULL
	};
	res = narg_argparse((char**)argv_enosuch, retv, optv, ARRAY_SIZE(optv), 2, ~0);
	expect_i(status, res.err, NARG_ENOSUCHOPTION);
	expect_i(status, res.arg, 1);

	const char *argv_emissing[] = {
		NULL,
		"-o",
		NULL
	};
	res = narg_argparse((char**)argv_emissing, retv, optv, ARRAY_SIZE(optv), 2, ~0);
	expect_i(status, res.err, NARG_EMISSINGPARAM);
	expect_i(status, res.arg, 1);

	const char *argv_eunexpect[] = {
		NULL,
		"--help=4",
		NULL
	};
	res = narg_argparse((char**)argv_eunexpect, retv, optv, ARRAY_SIZE(optv), 2, ~0);
	expect_i(status, res.err, NARG_EUNEXPECTEDPARAM);
	expect_i(status, res.arg, 1);

	const char *argv_eilseq[] = {
		NULL,
		"-\xE5",
		NULL
	};
	res = narg_argparse((char**)argv_eilseq, retv, optv, ARRAY_SIZE(optv), 2, ~0);
	expect_i(status, res.err, NARG_EILSEQ);
	expect_i(status, res.arg, 1);
}

int main(){
	int status = 0;
	ignore_rest(&status);
	max_positional(&status);
	flagorgy(&status);
	dashes0(&status);
	fail(&status);
	return status;
}

#endif //TEST
