#include <wchar.h>
#include <uchar.h>
#include <stdint.h>
#include <stdio.h>

#define c32rtomb(...) wcrtomb(__VA_ARGS__)

unsigned utf8charlen(wchar_t c){
	uint_fast8_t bits=7, bytes=1;
	while(c >= (1<<bits)){
		bytes++;
		bits = 5*bytes + 1;
	}
	return bytes;
}

unsigned mkutf8(char *res, char32_t tegn){
	unsigned bytes = utf8charlen(tegn);

	unsigned skreiv=0;
	char bitmask = (bytes == 1) ? 0 : ~(0xff >> bytes);
	res[skreiv++] = bitmask | (tegn >> (--bytes*6));
	while(bytes){
		res[skreiv++] = 0x80 | 0x3f & (tegn >> (--bytes*6));
	}
	return skreiv;
}

int main(){
	wchar_t combtab[4][2] = {
		{0x0300, 0x0370},
		{0x1DC0, 0x1E00},
		{0x20D0, 0x2100},
		{0xFE20, 0xFE30}
	};
	unsigned j;
	for(j=0; j<4; ++j){
		printf("%04x - %04x\n", combtab[j][0], combtab[j][1]);
		wchar_t comb;
		for(comb=combtab[j][0]; comb < combtab[j][1]; ++comb){
			char buf[6];
			buf[0] = ' ';
			unsigned len = 2 + mkutf8(buf+2, comb);
			char c;
			for(c='a'; c<='z'; ++c){
				buf[1] = c;
				fwrite(buf, 1, len, stdout);
			}
			puts("");
		}
	}
}