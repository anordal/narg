#include <stdio.h>

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

#define ARRAY_SIZE(a) (sizeof(a)/sizeof(a[0]))

static void compare_expr_u(int *status, const char *expr, unsigned actual, unsigned expected){
	if(actual != expected){
		fprintf(stderr, "%s: actual=%u, expected=%u\n", expr, actual, expected);
		*status = 1;
	}
}

#define expect_u(status, expr, expect) compare_expr_u(status, TOSTRING(expr), expr, expect)
