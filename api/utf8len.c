#include "utf8len.h"
#ifndef TEST
#include <stddef.h> //ptrdiff_t

int_fast8_t utf8len(const char *first){
	uint8_t leadbits=0;
	while(*first & ((uint8_t)'\x80' >> leadbits)) leadbits++;
	if(leadbits == 1){
		//not utf8
		return -1;
	}
	if(leadbits == 0){
		//ascii
		if(*first == '\0') return 0;
		leadbits = 1;
	}
	
	// Validate utf8
	const char *s = first;
	do{
		s++; //utf8 multibyte
	}while((*s & 0xC0) == 0x80);
	ptrdiff_t firstlen = s - first;
	if(firstlen != leadbits){
		//not utf8
		return -1;
	}
	
	// That was first codepoint. Now check if next is combining:
	leadbits=0;
	while(*s & ((uint8_t)'\x80' >> leadbits)) leadbits++;
	if(leadbits == 0){
		//ascii: Clearly not
		return firstlen;
	}
	
	// Get codepoint, validate utf8
	uint_fast8_t mask = ((uint8_t)'\x80' >> leadbits) - 1;
	uint_fast32_t code = 0;
	do{
		code <<= 6;
		code |= (*s++ & mask);
		mask = 0x3f;
	}while((*s & 0xC0) == 0x80);
	if(s - (first+firstlen) != leadbits){
		//not utf8
		return -1;
	}
	
	if(code < 0x0300 || 0x0370 <= code) //Diacritical Marks                  0011 0xxX xxxx  1100 110x  10xX xxxx
	if(code < 0x1DC0 || 0x1E00 <= code) //Diacritical Marks Supplement  0001 1101 11xx xxxx  1110 0001  1011 0111  10xx xxxx
	if(code < 0x20D0 || 0x2100 <= code) //Diacritical Marks for Symbols 0010 0000 1101 xxxx  1110 0010  1000 0011  1010 xxxx
	if(code < 0xFE20 || 0xFE30 <= code) //Half Marks                    1111 1110 0010 xxxx  1110 1111  1011 1000  1010 xxxx
	return firstlen;
	return s - first;
}

#else //TEST
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
#endif //TEST
