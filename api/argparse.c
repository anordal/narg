
#include "narg.h"
#include "utf8len.h"
#include <stddef.h>
#include <assert.h>

#include "../inc/str.c"
#include "../inc/reverse_slices.c"
#include "../inc/assign_params.c"

const struct _narg_special_values_for_metavar narg_metavar = {0};

// const_cast
static const char **const_charpp(char **arg) { return (const char**)arg; }

struct narg_result
narg_argparse(
	char **argv,
	struct narg_paramret *retv,
	const struct narg_optspec *optv,
	unsigned optc,
	unsigned dashes_longopt,
	unsigned max_positional_args)
{
	const unsigned dashes_shortopt = (dashes_longopt > 1)
		? dashes_longopt - 1
		: dashes_longopt
	;

	unsigned positional_begin=0, positional_end=~0;
	for (unsigned a=1; ; ++a) {
		const char *arg = argv[a];

		if (arg == NULL) {
ignore_rest:
			max_positional_args = 0;
positional_argument:
			if (!positional_begin) {
				positional_begin = a;
			}
			//BUG: Moving opts after referencing params. Solution: Treat opts, not posargs.
			else if (positional_end < a) {
				reverse_slices(
					const_charpp(argv + positional_begin),
					positional_end - positional_begin,
					a - positional_begin
				);
				positional_begin += a - positional_end;
			}
			positional_end = a + 1;
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

		const unsigned len_paramsep = (dashcount >= dashes_longopt)
			? 1 // strlen("=")
			: 0 // strlen("")
		;
		const unsigned expectedlen = (len_paramsep)
			? strlen_delim(arg, '=')
			: utf8len(arg)
		;
		unsigned o;
		for (o=0; o != optc; o++) {
			if (dashcount >= dashes_longopt) {
				if (has_prefix_of_len(arg, optv[o].longopt, expectedlen)) break;
			}
			if (dashcount == dashes_shortopt) {
				if (has_prefix_of_len(arg, optv[o].shortopt, expectedlen)) break;
			}
		}
		if (o == optc) {
			struct narg_result ret = { .err=NARG_ENOSUCHOPTION, .arg=a };
			return ret;
		}

		const unsigned params = narg_wordcount(optv[o].metavar);
		const unsigned num_firstparam = (arg[expectedlen])
			? 1 // the first parameter is in the option argument
			: 0 // zero parameters in the option argument
		;
		const unsigned pos_firstparam = (arg[expectedlen])
			? dashcount + expectedlen + len_paramsep
			: 0 // noop
		;
		unsigned paramcount = num_firstparam;
		char **paramvec = argv + a + 1 - num_firstparam;
		while (paramcount < params && paramvec[paramcount]) {
			++paramcount;
		}
		if (paramcount != params) {
			struct narg_result ret = {
				.err = (paramcount < params) ? NARG_EMISSINGPARAM : NARG_EUNEXPECTEDPARAM,
				.arg = a
			};
			return ret;
		}
		paramvec[0] += pos_firstparam;
		assign_params(const_charpp(argv), retv, optc, o, 1-num_firstparam, paramcount, const_charpp(paramvec));

		a += paramcount - num_firstparam;

		if (optv[o].metavar == &narg_metavar.ignore_rest){
			arg = argv[++a];
			goto ignore_rest;
		}
	}

	struct narg_result ret = { .err=0, .arg=positional_begin };
	return ret;
}

// static unsigned lookup(
// 	const struct optspec *haystack,
// 	char *needle,
// 	unsigned hayght,
// 	unsigned needlen,
// 	unsigned optsel
// ){
// 	unsigned optnum;
// 	for(optnum=0; optnum != hayght; optnum++){
// 		const char *cand = *(char**)((char*)(haystack+optnum) + optsel); //candidate
// 		if(!cand) continue;
//
// 		unsigned c=0;
// 		while(c < needlen && cand[c] == needle[c]) c++;
// 		if(c == needlen && cand[c] == '\0') break;
// 	}
// 	return optnum;
// }

// static unsigned leadingdashes(const char *s){
// 	unsigned count = 0;
// 	while(s[count] == '-' && s[count+1] != '\0') count++;
// 	return count;
// }

/* How to sort options in front of non-options:
 * While parsing options, move non-options since last option forward.
 *
 * return the index of the first positional argument.
 */

// struct narg_result
// narg(
// 	const struct optspec *optv,
// 	const char *const *argv,
// 	unsigned dashes,
// 	unsigned max_positional_args)
// {
// 	unsigned n=0, firstnonopt=~0, lastnonopt=~0;
// 	for(;;){
// 		n++;
// 		if(argv[n] == NULL) goto nonopt;
//
// 		unsigned dashcount = leadingdashes(argv[n]);
// 		unsigned optnum, optlen;
// 		char *findme = argv[n] + dashcount;
// 		if(dashes != 0){
// 			if(dashcount == 0){
// nonopt:
// 				if(lastnonopt+1 != n){
// 					// TODO: Is it a problem to discard opt? Yes if returning pointers into argv.
// 					// adjmemswap instead
// 					unsigned r=lastnonopt+1, w=n;
// 					while(r > firstnonopt){
// 						argv[--w] = argv[--r];
// 					}
// 					firstnonopt = w;
// 				}
// 				if(n == argc) break;
// 				lastnonopt = n;
// 				continue;
// 			}
// 			_Bool longopt = dashcount >= dashes;
// 			if(longopt){
// 				optlen = optionlen(findme);
// 				optnum = lookup_united(optv, findme, optc, optlen, offsetof(struct optspec, longopt));
// 			}else{
// 				shortopt:
// 				optlen = utf8len(findme);
// 				optnum = lookup_united(optv, findme, optc, optlen, offsetof(struct optspec, shortopt));
// 			}
// 		}else{
// 			if(findme[0] == '-') findme = argv[n];
// 			optlen = optionlen(findme);
// 			optnum = lookup_united(optv, findme, optc, optlen, offsetof(struct optspec, shortopt));
// 			if(optnum == optc){
// 				optnum = lookup_united(optv, findme, optc, optlen, offsetof(struct optspec, longopt));
// 			}
// 		}
//
// 		if(optnum == optc){
// 			findme[optlen] = '\0';
// 			argv[n] = findme;
// 			struct result_united ret = {.err=ENOSUCHOPTION, .arg=n};
// 			return ret;
// 		}
//
// 		// Forenklingar mot forviklingar
// 		// Den derre erlik-syntaksen, skal kanskje forbeholdes nparams=1,
// 		//  samt bare gå an å kombinere med nparams=0.
// 		// nparams=many kan ikke kombineres med nparams!=0
// 		if(optv[optnum].nparams == -1) argc = n+1; //this is an opt, the next is end
// 		if(optv[optnum].nparams == -2) goto nonopt;
// 		if(optv[optnum].nparams == 0){
// 			if(findme[optlen] == '='){
// 				argv[n] = findme;
// 				struct result_united ret = {.err=EUNEXPECTEDPARAM, .arg=n};
// 				return ret;
// 			}
// 			ans[optnum] = findme;
// 		}
// 		if(optv[optnum].nparams == 1){
// 			if(findme[optlen] == '='){
// 				ans[optnum] = findme + optlen;
// 				break;
// 			}else if(argv[n+1]){
// 				ans[optnum] = argv[++n];
// 			}else{
// 				findme[optlen] = '\0';
// 				argv[n] = findme;
// 				struct result_united ret = {.err=EMISSINGPARAM, .arg=n};
// 				return ret;
// 			}
// 		}
// 		//TODO: nparams>1, nparams=varying
//
// 		findme += optlen;
// 		if(*findme && *findme != '=') goto shortopt;
// 	}
// 	struct result_united ret = {.err=0, .arg=firstnonopt};
// 	return ret;
// }
