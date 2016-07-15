#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

#define ARRAY_SIZE(a) (sizeof(a)/sizeof(a[0]))

void compare_expr_u(int *status, const char *expr, unsigned actual, unsigned expected);
void compare_expr_i(int *status, const char *expr, signed actual, signed expected);

#define expect_u(status, expr, expect) compare_expr_u(status, TOSTRING(expr), expr, expect)
#define expect_i(status, expr, expect) compare_expr_i(status, TOSTRING(expr), expr, expect)

void compare_slices(int *status, const char **a, const char **b, unsigned len);

struct narg_paramret;
void expect_paramret(int *status, const struct narg_paramret *actual, unsigned expectedlen, const char **expected);
