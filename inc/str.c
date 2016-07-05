
static unsigned leadingchars(const char *s, char c)
{
	unsigned i = 0;
	while (s[i] == c) i++;
	return i;
}

static unsigned strlen_delim(const char *s, char delim)
{
	unsigned i = 0;
	while (s[i] && s[i] != delim) ++i;
	return i;
}

static unsigned longest_common_prefix(const char *a, const char *b)
{
	unsigned i = 0;
	while (a[i] && a[i] == b[i]) ++i;
	return i;
}

static _Bool has_prefix_of_len(const char *has, const char *candidate, unsigned expectedlen)
{
	if (!candidate) return 0;

	const unsigned pivot = longest_common_prefix(has, candidate);
	return (candidate[pivot] == '\0' && pivot == expectedlen);
}

#ifdef TEST
#include <stdbool.h>
#include "macros_test.h"

int main()
{
	int status = 0;
	expect_u(&status, leadingchars("", '-'), 0);
	expect_u(&status, leadingchars("a", '-'), 0);
	expect_u(&status, leadingchars("-", '-'), 1);
	expect_u(&status, leadingchars("--abc---", '-'), 2);
	expect_u(&status, leadingchars("-----abc", '-'), 5);

	expect_u(&status, strlen_delim("", '='), 0);
	expect_u(&status, strlen_delim("=", '='), 0);
	expect_u(&status, strlen_delim("abc", '='), 3);
	expect_u(&status, strlen_delim("abc=", '='), 3);
	expect_u(&status, strlen_delim("=abc", '='), 0);
	expect_u(&status, strlen_delim("abc=def", '='), 3);

	expect_u(&status, longest_common_prefix("", ""), 0);
	expect_u(&status, longest_common_prefix("abc", ""), 0);
	expect_u(&status, longest_common_prefix("", "def"), 0);
	expect_u(&status, longest_common_prefix("abc", "def"), 0);
	expect_u(&status, longest_common_prefix("abc", "abc"), 3);
	expect_u(&status, longest_common_prefix("abc", "abcdef"), 3);
	expect_u(&status, longest_common_prefix("abcdef", "abc"), 3);

	expect_u(&status, has_prefix_of_len("", "", 0), true);
	expect_u(&status, has_prefix_of_len("abc", "abc", 3), true);
	expect_u(&status, has_prefix_of_len("abc", "def", 3), false);
	expect_u(&status, has_prefix_of_len("abc", "abcdef", 3), false);
	expect_u(&status, has_prefix_of_len("abcdef", "abc", 3), true);
	expect_u(&status, has_prefix_of_len("abcdef", "abc", 6), false);

	return status;
}
#endif //TEST
