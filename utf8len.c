#include "utf8len.h"
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
