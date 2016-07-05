#include "narg.h"
#ifndef TEST

unsigned narg_wordcount(const char *s)
{
	if (!s) return 0;

	unsigned count = 0;
	char prev = ' ';
	for (; *s; ++s){
		if (*s != ' ' && prev == ' ') count++;
		prev = *s;
	}
	return count;
}

#else //TEST
#include "../inc/macros_test.h"

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
#endif //TEST
