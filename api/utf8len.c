// This Source Code Form is subject to the terms
// of the Mozilla Public License, v. 2.0.
// If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "narg.h"
#ifndef TEST
#include <stddef.h> //ptrdiff_t

int_fast8_t narg_utf8len(const char *first) {
	uint8_t leadbits = 0;
	while (*first & ((uint8_t)'\x80' >> leadbits)) {
		leadbits++;
	}
	if (leadbits <= 1) {
		if (leadbits == 1) {
			//lost in mbseq
			return -1;
		}
		//leadbits == 0: ascii
		if (*first == '\0') {
			return 0;
		}
		leadbits = 1;
	}
	
	// Validate utf8
	const char *s = first;
	do {
		s++; //utf8 multibyte
	} while ((*s & 0xC0) == 0x80);
	ptrdiff_t lensofar = s - first;
	if (lensofar != leadbits) {
		//unexpected length of mbseq
		return -1;
	}
	
	// That was first codepoint
	for (;;) {
		// Check if next is combining
		leadbits=0;
		while (*s & ((uint8_t)'\x80' >> leadbits)) {
			leadbits++;
		}
		// All leadbits==1 bytes already swallowed by the first codepoint
		if (leadbits == 0) {
			//ascii
			return lensofar;
		}

		// Get codepoint, validate utf8
		uint_fast8_t mask = ((uint8_t)'\x80' >> leadbits) - 1;
		uint_fast32_t code = 0;
		do {
			code <<= 6;
			code |= (*s++ & mask);
			mask = 0x3f;
		} while ((*s & 0xC0) == 0x80);
		if (s - (first+lensofar) != leadbits) {
			//unexpected length of mbseq
			return -1;
		}

		if (code < 0x0300 || 0x0370 <= code) //Diacritical Marks                  0011 0xxX xxxx  1100 110x  10xX xxxx
		if (code < 0x1DC0 || 0x1E00 <= code) //Diacritical Marks Supplement  0001 1101 11xx xxxx  1110 0001  1011 0111  10xx xxxx
		if (code < 0x20D0 || 0x2100 <= code) //Diacritical Marks for Symbols 0010 0000 1101 xxxx  1110 0010  1000 0011  1010 xxxx
		if (code < 0xFE20 || 0xFE30 <= code) //Half Marks                    1111 1110 0010 xxxx  1110 1111  1011 1000  1010 xxxx
		break;

		lensofar = s - first;
	}
	return lensofar;
}

#else //TEST
#include "../testapi/testability.h"

int main() {
	struct {
		const char * input;
		int expected;
	} cases[] = {
		{"", 0}, //ascii len=0
		{"a", 1}, //ascii len=1
		{"aa", 1}, //ascii len=2
		{"\x80 ", -1}, //lost in mbseq
		{"a\x80 ", -1}, //too long mbseq
		{"\xE6 ", -1}, //incomplete mbseq 1
		{"a\xE6 ", -1}, //incomplete mbseq 2
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
		{"æ\u0307\u0327 ", 6}, //precomposed + composed + composed
		{"€\u20d7\u20d7 ", 9}, //precomposed + composed + composed
		{0, 0}
	};

	int status=0;
	for (unsigned i=0; cases[i].input; ++i) {
		int actual = narg_utf8len(cases[i].input);
		compare_expr_i(&status, cases[i].input, actual, cases[i].expected);
	}
	return status;
}
#endif //TEST
