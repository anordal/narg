#include "../utf8len.h"
#include <stdio.h>

int main(){
	struct{
		const char * input;
		int expected;
	} cases[] = {
		{"", 0}, //ascii len=0
		{"a", 1}, //ascii len=1
		{"aa", 1}, //ascii len=2
		{"\x80 ", -1}, //lost in mbseq
		{"\xE6 ", -1}, //incomplete mbseq
		{"æ ", 2},
		{"ø ", 2},
		{"å ", 2},
		{"¬ ", 2},
		{"€ ", 3},
		{"→ ", 3},
		{"⎟ ", 3},
		
		{"a\u02ff ", 1}, //not composed
		{"a\u0300 ", 3}, //    composed
		{"a\u036f ", 3}, //    composed
		{"a\u0370 ", 1}, //not composed
		
		{"a\u1dbf ", 1}, //not composed
		{"a\u1dc0 ", 4}, //    composed
		{"a\u1dff ", 4}, //    composed
		{"a\u1e00 ", 1}, //not composed
		
		{"a\u20cf ", 1}, //not composed
		{"a\u20d0 ", 4}, //    composed
		{"a\u20ff ", 4}, //    composed
		{"a\u2100 ", 1}, //not composed
		
		{"a\ufe1f ", 1}, //not composed
		{"a\ufe20 ", 4}, //    composed
		{"a\ufe2f ", 4}, //    composed
		{"a\ufe30 ", 1}, //not composed
		
		{"æ\u20d7 ", 5}, //precomposed + composed
		{"€\u20d7 ", 6}, //precomposed + composed
		{0, 0}
	};
	
	unsigned i;
	int actual;
	int status=0;
	for(i=0; cases[i].input; ++i){
		actual = utf8len(cases[i].input);
		if(actual != cases[i].expected){
			fprintf(stderr, "%s: expected=%d actual=%d\n", cases[i].input, cases[i].expected, actual);
			status = 1;
		}
	}
	return status;
}
