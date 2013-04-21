
/* A new take at getopt_long
 *
 * Rationale:
 * Unite implementation, shortopt, longopt, description.
 * Reentrancy.
 */
#ifdef __linux__
# include <sys/ioctl.h>
# include <unistd.h>
#endif
#include <stdlib.h> //wctomb
#include <string.h> //strlen
#include <ctype.h>  //isspace
#include <getopt-ng.h>

/* How to sort options in front of non-options:
 * While parsing options, find the first non-option and the last option.
 * while(first non-option < last option) intermediate non-options >>= 1.
 *
 * return index of the first non-option argument.
 */
unsigned getopt_ng(unsigned argc, char *argv[], struct optdesc flag[]){
	unsigned n, firstnonopt=~0, lastnonopt=~0;
	for(n=1; ; n++){
		if(n == argc || argv[n][0] != '-'){
			if(lastnonopt+1 != n){
				unsigned r=lastnonopt+1, w=n;
				while(r > firstnonopt){
					argv[--w] = argv[--r];
				}
				firstnonopt = w;
			}
			if(n == argc) break;
			lastnonopt = n;
			continue;
		}
		char *opt = argv[n]+1;
		struct optdesc *f;
		if(opt[0] == '-' && opt[1]){
			//longopt
			opt++;
			char *arg = NULL;
			for(f=flag; f->shortopt != '-'; f++){
				unsigned s=0;
				char *cmp = f->longopt;
				if(!cmp) continue;
				while(cmp[s] && opt[s] == cmp[s]) s++;
				if(
					cmp[s] == '\0' && (opt[s] == '\0' ||
					(opt[s] == '=' && f->argtempl))
				){
					if(opt[s] == '=') arg = opt + s + 1;
					break;
				}
			}
			if(f->argtempl){
				if(arg){
					f->ans = arg;
				}else{
					if(n+1 < argc) f->ans = argv[++n];
				}
			}else{
				f->ans = opt;
			}
		}else{
			//shortopt TODO: grok utf-8
			for(; *opt; opt++){
				for(f=flag; f->shortopt != '-'; f++){
					if(*opt == f->shortopt) break;
				}
				if(*opt == f->shortopt){
					if(f->shortopt == '-'){
						n = argc - 1;
					}else{
						if(f->argtempl){
							if(n+1 < argc) f->ans = argv[++n];
						}else{
							f->ans = opt;
						}
					}
				}else{
					opt[1] = '\0';
					f->ans = opt;
				}
			}
		}
	}
	return firstnonopt;
}

unsigned terminalwidth(){
	unsigned width = 80;
#ifdef __linux__
	struct winsize w;
	if(0 == ioctl(STDOUT_FILENO, TIOCGWINSZ, &w)) width = w.ws_col;
#endif
	return width;
}

typedef struct{
	unsigned x1, x2;
} pair_t;

static pair_t findbreakpoint(char *line, unsigned width){
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

void indentputs_unlocked(
	FILE *fp, unsigned *posPtr,
	unsigned indent, unsigned width, char *str
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

static pair_t utf8charlen(wchar_t c){
	pair_t ret = {1, 1};
	unsigned leadbits=7;
	if(c >= (1<<leadbits)){
		c >>= 6;
		ret.x1++;
		for(leadbits=5; leadbits; leadbits--){
			if(c < (1<<leadbits)) break;
			c >>= 6;
			ret.x1++;
		}
	}
	return ret;
}

static pair_t utf8strlen(char *str){
	pair_t ret = {0, 0};
	for(; str[ret.x1]; ret.x1++){
		if((str[ret.x1] & 0xC0) != 0x80) ret.x2++;
	}
	return ret;
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

void printopt_unlocked(FILE *fp, struct optdesc flag[], unsigned linelen){
	struct optdesc *f;
	pair_t shortlen={0}, longlen={0};
	for(f=flag; f->shortopt != '-'; f++){
		pair_t test = {0};
		if(f->shortopt) test = pairaddscalar(utf8charlen(f->shortopt), 2);
		shortlen = pairmax(shortlen, test);
		if(f->longopt)  test = pairaddscalar(utf8strlen(f->longopt), 2);
		if(f->argtempl) test = pairaddpair(test, utf8strlen(f->argtempl));
		longlen = pairmax(longlen, test);
	}

	//2 spaces before desc
	pair_t prelen = pairaddscalar(pairaddpair(shortlen, longlen), 2);
	unsigned indent = prelen.x2 % linelen;
	char preprint[prelen.x1]; //unterminated
	
	for(f=flag;; f++){
		pair_t wpos = {0};
		if(shortlen.x1){
			if(f->shortopt){
				preprint[wpos.x1++] = '-';
				wpos.x1 += wctomb(preprint+wpos.x1, f->shortopt);
				preprint[wpos.x1++] = ' ';
			}else{
				memset(preprint, ' ', shortlen.x2);
				wpos.x1 += shortlen.x2;
			}
			wpos.x2 += shortlen.x2;
		}
		if(f->longopt){
			preprint[wpos.x1++] = '-';
			preprint[wpos.x1++] = '-';
			wpos.x2 += 2;
			wpos = pairaddpair(wpos, utf8strcpy(preprint + wpos.x1, f->longopt));
		}
		if(f->argtempl){
			wpos = pairaddpair(wpos, utf8strcpy(preprint + wpos.x1, f->argtempl));
		}
		fwrite_unlocked(preprint, 1, wpos.x1, fp);
		wpos.x2 %= linelen;
		
		if(f->desc) indentputs_unlocked(fp, &wpos.x2, indent, linelen, f->desc);
		if(f->ans){
			char buf[strlen(f->ans) + 4];
			sprintf(buf, " [%s]", f->ans);
			indentputs_unlocked(fp, &wpos.x2, indent, linelen, buf);
		}
		putchar_unlocked('\n');
		if(f->shortopt == '-') break;
	}
}
