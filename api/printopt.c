#define _DEFAULT_SOURCE //fileno, fwrite_unlocked

#include "narg.h"

#ifdef __linux__
# include <sys/ioctl.h>
# include <unistd.h>
#endif
#include <string.h> //strlen
#include <ctype.h>  //isspace
unsigned narg_terminalwidth(FILE *fp){
	unsigned width = 80;
#ifdef __linux__
	int fd = fileno(fp);
	struct winsize w;
	if(0 == ioctl(fd, TIOCGWINSZ, &w)) width = w.ws_col;
#endif
	return width;
}

typedef struct{
	unsigned x1, x2;
} pair_t;

static pair_t findbreakpoint(const char *line, unsigned width){
	pair_t pos={0, 0}, breakpoint={~0, ~0};
	for(;;){
		if(line[pos.x1] == '\0' || isspace(line[pos.x1])){
			breakpoint = pos;
			if(line[pos.x1] == '\0' || line[pos.x1] == '\n') break;
		}
		if(pos.x2 >= width) break;
		pos.x2++;
		//swallow multibyte characters whole
		if((line[pos.x1] & 0xC0) == 0xC0) do{
			pos.x1++; //utf8 multibyte
		}while((line[pos.x1] & 0xC0) == 0x80);
		else pos.x1++;
	}
	return (breakpoint.x2 != ~0) ? breakpoint : pos;
}

void narg_indentputs_unlocked(
	FILE *fp, unsigned *posPtr,
	unsigned indent, unsigned width, const char *str
){
	unsigned pos = *posPtr;
	for(;;){ //line by line
		for(; pos < indent; pos++) putchar_unlocked(' ');
		pair_t breakpoint = findbreakpoint(str, width - pos);
		fwrite_unlocked(str, 1, breakpoint.x1, fp);
		str += breakpoint.x1;
		pos += breakpoint.x2;
		if(*str == '\0') break;
		if(isspace(*str)) str++;
		putchar_unlocked('\n');
		pos = 0;
	}
	*posPtr = pos;
}

static pair_t utf8strlen(const char *str){
	pair_t ret = {0, 0};
	for(; str[ret.x1]; ret.x1++){
		if((str[ret.x1] & 0xC0) != 0x80) ret.x2++;
	}
	return ret;
}

static pair_t nullutf8strlen(const char *s){
	if(s) return utf8strlen(s);
	pair_t niks = {0};
	return niks;
}

static pair_t utf8strcpy(char *restrict dst, const char *restrict src){
	pair_t i = {0, 0};
	for(; src[i.x1]; i.x1++){
		dst[i.x1] = src[i.x1];
		if((src[i.x1] & 0xC0) != 0x80) i.x2++;
	}
	return i;
}

static pair_t pairmax(pair_t a, pair_t b){
	if(a.x1 < b.x1) a.x1 = b.x1;
	if(a.x2 < b.x2) a.x2 = b.x2;
	return a;
}

static pair_t pairaddscalar(pair_t a, unsigned b){
	a.x1 += b;
	a.x2 += b;
	return a;
}

static pair_t pairaddpair(pair_t a, pair_t b){
	a.x1 += b.x1;
	a.x2 += b.x2;
	return a;
}

void narg_printopt_unlocked(
	FILE *fp,
	const char ***ansv,
	const struct narg_optspec *optv,
	unsigned optc,
	unsigned dashes_longopt,
	unsigned width
){
	const unsigned dashes_shortopt = (dashes_longopt > 1)
		? dashes_longopt - 1
		: dashes_longopt
	;

	pair_t shortlen={0}, longlen={0};
	unsigned o;
	for(o=0; o < optc; o++){
		shortlen = pairmax(shortlen, nullutf8strlen(optv[o].shortopt));
		longlen = pairmax(longlen, pairaddpair(nullutf8strlen(optv[o].longopt), nullutf8strlen(optv[o].metavar)));
	}
	if(shortlen.x2) shortlen = pairaddscalar(shortlen, dashes_shortopt);
	if(shortlen.x2 && longlen.x2) shortlen = pairaddscalar(shortlen, 1);
	if(longlen.x2) longlen = pairaddscalar(longlen, dashes_longopt);
	longlen = pairaddscalar(longlen, 2); //2 spaces before help

	pair_t prelen = pairaddpair(shortlen, longlen);
	unsigned indent = prelen.x2 % width;
	char preprint[prelen.x1]; //unterminated
	
	for(o=0; o < optc; o++){
		pair_t wpos = {0};

		if(optv[o].shortopt){
			memset(preprint + wpos.x1, '-', dashes_shortopt);
			wpos = pairaddscalar(wpos, dashes_shortopt);
			wpos = pairaddpair(wpos, utf8strcpy(preprint + wpos.x1, optv[o].shortopt));
		}
		memset(preprint + wpos.x1, ' ', shortlen.x2 - wpos.x2);
		wpos = pairaddscalar(wpos, shortlen.x2 - wpos.x2);

		if(optv[o].longopt){
			memset(preprint + wpos.x1, '-', dashes_longopt);
			wpos = pairaddscalar(wpos, dashes_longopt);
			wpos = pairaddpair(wpos, utf8strcpy(preprint + wpos.x1, optv[o].longopt));
		}else{
			memset(preprint + wpos.x1, ' ', dashes_longopt);
			wpos = pairaddscalar(wpos, dashes_longopt);
		}
		if(optv[o].metavar){
			wpos = pairaddpair(wpos, utf8strcpy(preprint + wpos.x1, optv[o].metavar));
		}
		memset(preprint + wpos.x1, ' ', prelen.x2 - wpos.x2);
		wpos = pairaddscalar(wpos, prelen.x2 - wpos.x2);

		fwrite_unlocked(preprint, 1, wpos.x1, fp);
		wpos.x2 %= width;
		
		if(optv[o].help) narg_indentputs_unlocked(fp, &wpos.x2, indent, width, optv[o].help);
		if(ansv[o]){
			unsigned num_args = narg_wordcount(optv[o].metavar);
			if(num_args){
				char buf[100];
				char *const end = buf + sizeof(buf);
				char *pos = stpcpy(buf, " [");
				for(unsigned a=0; ;){
					pos = stpncpy(pos, ansv[o][a++], end-pos);
					if(a == num_args || end-pos < 2) break;
					*pos++ = ' ';
				}
				if(end-pos < 2){
					pos = stpcpy(end-2-strlen("…"), "…");
				}
				stpcpy(pos, "]");
				narg_indentputs_unlocked(fp, &wpos.x2, indent, width, buf);
			}
		}
		putchar_unlocked('\n');
	}
}
