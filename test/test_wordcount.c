#include "../narg.h"

static void expect_u_test(int *status, const char *x, unsigned actual, unsigned expected){
	if(actual != expected){
		fprintf(stderr, "%s: actual=%u, expected=%u\n", x, actual, expected);
		*status = 1;
	}
}

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define expect_u(status, expr, expect) expect_u_test(status, TOSTRING(expr), expr, expect)

int main(){
	int status = 0;
	expect_u(&status, narg_wordcount(NULL), 0);
	expect_u(&status, narg_wordcount(""), 0);
	expect_u(&status, narg_wordcount(" "), 0);
	expect_u(&status, narg_wordcount("a"), 1);
	expect_u(&status, narg_wordcount(" a "), 1);
	expect_u(&status, narg_wordcount("a b"), 2);
	expect_u(&status, narg_wordcount("a b c"), 3);
	return status;
}
