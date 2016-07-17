// This Source Code Form is subject to the terms
// of the Mozilla Public License, v. 2.0.
// If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

static unsigned narg_wordcount(const char *s) {
	if (!s) {
		return 0;
	}

	unsigned count = 0;
	char prev = ' ';
	for (; *s; ++s) {
		if (*s != ' ' && prev == ' ') count++;
		prev = *s;
	}
	return count;
}

#ifdef TEST
#undef TEST
#include <stddef.h> //NULL
#include "../testapi/testability.h"

int main() {
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
#endif //TEST
