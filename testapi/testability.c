#define _DEFAULT_SOURCE //unlocked stdio
#include "testability.h"

#ifndef TEST
#include <string.h>
#include "../api/narg.h"

void compare_expr_u(int *status, const char *expr, unsigned actual, unsigned expected){
	if(actual != expected){
		fprintf(stderr, "%s: actual=%u, expected=%u\n", expr, actual, expected);
		*status = 1;
	}
}

void compare_expr_i(int *status, const char *expr, signed actual, signed expected){
	if(actual != expected){
		fprintf(stderr, "%s: actual=%i, expected=%i\n", expr, actual, expected);
		*status = 1;
	}
}

static size_t fputs_unlocked(const char *s, FILE *fp){
	return fwrite_unlocked(s, 1, strlen(s), fp);
}

// Precondition: len > 0
static void slice_fprint_unlocked(FILE *fp, const char **slice, unsigned len){
	fputc_unlocked('{', fp);
	for(;;){
		fputs_unlocked(*slice++, fp);
		if(--len == 0) break;
		fputc_unlocked(',', fp);
	}
	fputc_unlocked('}', fp);
}

static int slicecmp(const char **a, const char **b, unsigned len){
	int diff = 0;
	for(unsigned i=0; i!=len; ++i){
		diff = strcmp(a[i], b[i]);
		if(diff) break;
	}
	return diff;
}

void compare_slices(int *status, const char **a, const char **b, unsigned len){
	if(0 != slicecmp(a, b, len)){
		flockfile(stderr);
		slice_fprint_unlocked(stderr, a, len);
		fputs_unlocked(" != ", stderr);
		slice_fprint_unlocked(stderr, b, len);
		fputc_unlocked('\n', stderr);
		funlockfile(stderr);
		*status = 1;
	}
}

void expect_paramret(int *status, const struct narg_paramret *actual, unsigned expectedlen, const char **expected)
{
	int lenstatus = 0;
	compare_expr_u(&lenstatus, "expect_paramret", actual->paramc, expectedlen);
	*status |= lenstatus;
	if(lenstatus == 0){
		compare_slices(status, actual->paramv, expected, expectedlen);
	}
}

#else //TEST
#include <stdio.h>

static void fpperror(int *status, FILE *fp, const char *msg){
	if(fp == NULL){
		perror(msg);
		*status = 1;
	}
}

static void test_failing(int *status){
	int invstatus = 0; //result of trying to fail
	fpperror(&invstatus, NULL, "");
	invstatus ^= 1;
	*status |= invstatus;

	invstatus = 0;
	compare_expr_u(&invstatus, "", 0, 1);
	invstatus ^= 1;
	*status |= invstatus;

	invstatus = 0;
	compare_expr_i(&invstatus, "", 1, 0);
	invstatus ^= 1;
	*status |= invstatus;

	invstatus = 0;
	compare_slices(&invstatus, (const char *[]){"a","b","c","c"}, (const char *[]){"a","b","c","d"}, 4);
	invstatus ^= 1;
	*status |= invstatus;
}

int main(){
	int status = 0;
	freopen("/dev/null", "a", stderr);
	fpperror(&status, stderr, "freopen: /dev/null");

	// pyramid-of-doom style success checks
	// instead of failure checks makes
	// the coverage report happy on success
	if(stderr != NULL){
		test_failing(&status);
	}
	return status;
}

#endif //TEST
