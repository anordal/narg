#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <locale.h>
#include <ctype.h>
#include <handymacros.h>
#include "../narg.h"

//output formats
#define ANSI    0
#define WINDOZE 1
#define HTML    2
#define LATEX   3

#ifndef COLOR
# ifdef _WIN32
#  define COLOR WINDOZE
# else
#  define COLOR ANSI
# endif
#endif

#if COLOR == HTML
# define STATERECURSIVE_OUTPUT
# define HTMLELEMENT "pre"
#endif

#if COLOR == LATEX
# define STATERECURSIVE_OUTPUT
# ifndef STYLABLE
#  define STYLABLE
# endif
#endif

/* Problem:
 * Bit positions defined for red and blue in ansi are opposite from windows.h.
 * Html uses many bits, but same order as WINDOZE_COLORS.
 * Ansi color codes lack BACKGROUND_INTENSITY and have some ugly corner cases,
 * easy to use - hard to program for.
 *
 * Solution for now:
 * Conditional compilation. One program for each output.
 *
 * Verdict & future plan:
 * Ansi sucks, any runtime translation is deserved. Escape codes are portable.
 * cmd.exe sucks for other reasons. Who uses it anyway? Its api is unportable.
 * Colors should be defined in html for best precision and interchangeability.
 *
 * Implementation notes:
 * Colors can be statically downsampled from html to either ansi or win.
 * WINDOZE_COLORS are defined by constants, not numbers like html and ansi.
 */
#if COLOR == WINDOZE
# include <windows.h>
#else
# define FOREGROUND_RED       0x01
# define FOREGROUND_GREEN     0x02
# define FOREGROUND_BLUE      0x04
# define FOREGROUND_INTENSITY 0x08
# define BACKGROUND_RED       0x10
# define BACKGROUND_GREEN     0x20
# define BACKGROUND_BLUE      0x40
# define BACKGROUND_INTENSITY 0x80
#endif
#define FOREGROUND_YELLOW  (FOREGROUND_RED   | FOREGROUND_GREEN)
#define FOREGROUND_MAGENTA (FOREGROUND_RED   | FOREGROUND_BLUE )
#define FOREGROUND_CYAN    (FOREGROUND_GREEN | FOREGROUND_BLUE )
#define FOREGROUND_WHITE   (FOREGROUND_RED   | FOREGROUND_CYAN )
#define FOREGROUND     (FOREGROUND_INTENSITY | FOREGROUND_WHITE)
#define BACKGROUND_YELLOW  (BACKGROUND_RED   | BACKGROUND_GREEN)
#define BACKGROUND_MAGENTA (BACKGROUND_RED   | BACKGROUND_BLUE )
#define BACKGROUND_CYAN    (BACKGROUND_GREEN | BACKGROUND_BLUE )
#define BACKGROUND_WHITE   (BACKGROUND_RED   | BACKGROUND_CYAN )
#define BACKGROUND     (BACKGROUND_INTENSITY | BACKGROUND_WHITE)

/*
 * A state has a style, which has a name and a color.
 * As an optimisation, when !defined STYLABLE, we are
 * style agnostic and use the color in place of the style.
 * The style/color in struct state is for inheritance.
 */

typedef uint8_t statecode_t;
typedef uint8_t stylecode_t;
typedef uint8_t color_t;
#ifdef STYLABLE
typedef stylecode_t colyl_t;
#else
typedef color_t     colyl_t;
#endif

struct state{
	statecode_t state;
	colyl_t     style;
};

enum stylecode{
	STYLE_NONE,
	STYLE_STR,
	STYLE_CHR,
	STYLE_ESC,
	STYLE_COMMENT,
	STYLE_PREPROC,
	STYLE_DEC,
	STYLE_OCT,
	STYLE_HEX,
	STYLE_LINENUM,
	STYLE_INDENT1,
	STYLE_INDENT2,
	STYLE_BRC1,
	STYLE_BRC2,
	STYLE_BRC3,
	STYLE_BRC4,
	STYLE_BRC5,
	STYLE_BRC6,
	NUM_STYLES
};
#ifdef STYLABLE
static const char *stylename[NUM_STYLES] = {
	"NONE",
	"STR",
	"CHR",
	"ESC",
	"COMMENT",
	"PREPROC",
	"DEC",
	"OCT",
	"HEX",
	"LINENUM",
	"IN1",
	"IN2",
	//{}
	"BRC1",
	"BRC2",
	//()
	"BRC3",
	"BRC4",
	//[]
	"BRC5",
	"BRC6"
};
#endif
static const color_t stylecolor[NUM_STYLES] = {
	0,
	FOREGROUND_RED,
	FOREGROUND_MAGENTA,
	FOREGROUND_MAGENTA | FOREGROUND_INTENSITY,
	FOREGROUND_INTENSITY,
	FOREGROUND_GREEN,
	FOREGROUND_INTENSITY | FOREGROUND_BLUE,
	FOREGROUND_INTENSITY | FOREGROUND_CYAN,
	FOREGROUND_INTENSITY | FOREGROUND_CYAN,
	FOREGROUND_INTENSITY | FOREGROUND_YELLOW | BACKGROUND_WHITE,
	FOREGROUND_MAGENTA,
	FOREGROUND_CYAN,
	FOREGROUND_MAGENTA,
	FOREGROUND_CYAN,
	FOREGROUND_BLUE,
	FOREGROUND_YELLOW,
	FOREGROUND_RED,
	FOREGROUND_INTENSITY
};

enum statecode{
	STATE_NONE,
	STATE_IDENTIFIER,
	STATE_STR,
	STATE_CHR,
	STATE_ESC,
	STATE_CCOMMENT,
	STATE_CPPCOMMENT,
	STATE_PREPROC,
	STATE_DEC,
	STATE_OCT,
	STATE_HEX0X,
	STATE_HEX,
	STATE_BRACE,
	STATE_LINENUM,
	STATE_INDENT1,
	STATE_INDENT2,
	NUM_STATES
};
static stylecode_t statestyle[NUM_STATES] = {
	STYLE_NONE,
	STYLE_NONE,
	STYLE_STR,
	STYLE_CHR,
	STYLE_ESC,
	STYLE_COMMENT,
	STYLE_COMMENT,
	STYLE_PREPROC,
	STYLE_DEC,
	STYLE_OCT,
	STYLE_HEX,
	STYLE_HEX,
	STYLE_NONE,
	STYLE_LINENUM,
	STYLE_INDENT1,
	STYLE_INDENT2,
};
#ifndef STYLABLE
static color_t statecolor[NUM_STATES];
#endif

struct{
	const char *numformat;
	const char *tabstring;
	uint8_t numwidth;
	uint8_t width;
	_Bool doc;
	_Bool stylable;
} globset;

#define ROLLSIZE 7

struct stlist{
	struct stlist *prev;
	uint16_t rollfill;
	struct state roll[ROLLSIZE];
};

static struct stlist unpop;
static struct stlist *ststack = &unpop;
static struct state curstate;
static unsigned linenum;
static uint8_t togglebox = 0;

static void const_init(){
	unpop.prev = NULL;
	unpop.rollfill = 0;
	curstate.state = STATE_NONE;
#ifdef STYLABLE
	curstate.style = statestyle[curstate.state];
#else
	for(statecode_t steit=0; steit<NUM_STATES; steit++){
		statecolor[steit] = stylecolor[statestyle[steit]];
	}
	curstate.style = statecolor[curstate.state];
#endif
}

static void perfile_init(){
	linenum = 2;
}

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

#ifdef STYLABLE
// ansi color codes have rgb in reverse order.
static unsigned rgb12_from_ansi4(color_t c){
	unsigned rgb = ((c & (1<<0)) << 10) | ((c & (1<<1)) << 5) | (c & (1<<2));
	rgb = (rgb << 1) | (rgb >> 1);
	if(c >> 3) rgb |= 0x555;
	return rgb & 0xfff;
}
static unsigned rgb24_from_ansi4(color_t c){
	unsigned rgb = ((c & (1<<0)) << 18) | ((c & (1<<1)) << 9) | (c & (1<<2));
	rgb = (rgb << 1) | (rgb >> 1);
	if(c >> 3) rgb |= 0x050505;
	rgb |= rgb << 4;
	return rgb & 0xffffff;
}

static void htmlcolorcode(char *str, unsigned wpos, color_t dif, color_t col){
	if(dif){
		sprintf(str + wpos, "%03x;", rgb12_from_ansi4(col));
	}else{
		*str = '\0';
	}
}
static void color_html(color_t oldc, color_t newc){
	color_t diff = oldc ^ newc;
	char colstr[] = "color:#000;";
	char bgcolstr[] = "background-color:#000;";
	htmlcolorcode(colstr, 7, diff & 0xf, newc & 0xf);
	htmlcolorcode(bgcolstr, 18, diff >> 4, newc >> 4);
	printf("<span style=\"%s%s\">", colstr, bgcolstr);
}

static void color(stylecode_t oldyl, stylecode_t newyl){
# if COLOR == HTML
	if(globset.stylable){
		printf("<span class=\"%s\">", stylename[newyl]);
	}else{
		color_html(stylecolor[oldyl], stylecolor[newyl]);
	}
# elif COLOR == LATEX
	printf("{\\color{%s}", stylename[newyl]);
# endif
}
#else
static void color(color_t oldc, color_t newc){
# if COLOR == WINDOZE
	if((newc & FOREGROUND) == 0) newc |= FOREGROUND_WHITE;
	HANDLE handletur = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(handletur, newc);
# elif COLOR == ANSI
	color_t diff = oldc ^ newc;
	unsigned noskipmask = 0;
	if((diff & 0xf0) && !(newc & 0xf0)){
		//Trying to remove background color is futile. Must reset everything
		fputs("\e[m", stdout);
		diff = newc;
		if(!diff) return;
	}
	if((newc & 0x0f) == FOREGROUND_INTENSITY){
		//black foreground + FOREGROUND_INTENSITY means grey
		diff |= 0xf;
		noskipmask = (1 << 3) | (1 << 0);
	}else if((diff & FOREGROUND_INTENSITY) && !(newc & FOREGROUND_INTENSITY)){
		//for some reason, removing FOREGROUND_INTENSITY resets everything
		noskipmask = 1 << 0;                //reset everything
		diff = FOREGROUND_INTENSITY ^ newc; //account for that
	}
	diff &= 0x7f;
	color_t lookfor[] = {
		FOREGROUND_INTENSITY,
		0,
		0,
		FOREGROUND_WHITE,
		BACKGROUND_WHITE
	};
	char colstr[] = "\e[0;30;40m";
	char *colpos = colstr + 2;
	unsigned lookpos;
	for(lookpos=0; diff; lookpos++){
		color_t offer = lookfor[lookpos];
		if(diff & offer){
			diff &= ~offer;
			uint8_t ansi = newc & offer;
			while((offer & 1) == 0){
				offer >>= 1;
				ansi >>= 1;
			}
			if(!ansi && !(noskipmask >> lookpos)) continue;
			ansi |= (lookpos << 4);
			colpos += sprintf(colpos, "%x;", ansi);
		}
	}
	if(colpos != colstr + 2) colpos--;
	colpos[0] = 'm';
	colpos[1] = '\0';
	fputs(colstr, stdout);
# elif COLOR == HTML
	color_html(oldc, newc);
# endif
}
#endif //stylable

#ifdef STATERECURSIVE_OUTPUT
static void uncolor(colyl_t oldc, colyl_t newc){
# if COLOR == HTML
	printf("</span>");
# elif COLOR == LATEX
	printf("}");
# else
	color(oldc, newc);
# endif
}
#endif

void stpush(statecode_t newst){
	if(ststack->rollfill == ROLLSIZE){
		struct stlist *node = malloc(sizeof(struct stlist));
		if(!node){
			perror("malloc");
			exit(1);
		}
		node->rollfill = 0;
		node->prev = ststack;
		ststack = node;
	}
	ststack->roll[ststack->rollfill++] = curstate;

#ifdef STYLABLE
	colyl_t newyl = statestyle[newst];
#else
	colyl_t newyl = statecolor[newst];
#endif
	if(!newyl) newyl = curstate.style;
#ifdef STATERECURSIVE_OUTPUT
	if(newyl != curstate.style) color(curstate.style, newyl);
#endif
	curstate.state = newst;
	curstate.style = newyl;
}

void stpop(){
	struct state pop;
	if(ststack->rollfill == 0){
		if(ststack == &unpop){
			pop.state = STATE_NONE;
#ifdef STYLABLE
			pop.style = statestyle[STATE_NONE];
#else
			pop.style = statecolor[STATE_NONE];
#endif
			goto have_pop;
		}else{
			struct stlist *node = ststack;
			ststack = ststack->prev;
			free(node);
		}
	}
	pop = ststack->roll[--(ststack->rollfill)];
have_pop:
#ifdef STATERECURSIVE_OUTPUT
	if(pop.style != curstate.style) uncolor(curstate.style, pop.style);
#endif
	curstate.state = pop.state;
	curstate.style = pop.style;
}

static void enbrace(uint_fast8_t type, _Bool closing){
	stylecode_t bracestyle;
	if(!closing){
		bracestyle = togglebox;
		togglebox ^= 1 << type;
	}else{
		togglebox ^= 1 << type;
		bracestyle = togglebox;
	}
	bracestyle = STYLE_BRC1 + (type*2 | ((bracestyle >> type) & 1));
#ifdef STYLABLE
	statestyle[STATE_BRACE] = bracestyle;
#else
	statecolor[STATE_BRACE] = stylecolor[bracestyle];
#endif
	stpush(STATE_BRACE);
}

static void lazycolor(char c){
#ifndef STATERECURSIVE_OUTPUT
	static color_t lazy = 0;
	if(curstate.style == lazy) return;
	if(c <= ' ' && (curstate.style & BACKGROUND) == (lazy & BACKGROUND)) return;
	color(lazy, curstate.style);
	lazy = curstate.style;
#endif
}

static void output(char c);

static void lineenum(){
	stpush(STATE_LINENUM);
#ifndef STATERECURSIVE_OUTPUT
	lazycolor('%');
#endif
#if COLOR == LATEX
	char buf[20];
	snprintf(buf, 20, globset.numformat, linenum++);
	for(uint8_t i=0; i<4; i++) output(buf[i]);
#else
	printf(globset.numformat, linenum++);
#endif
}

static void output(char c){
	if(curstate.state == STATE_INDENT1 || curstate.state == STATE_INDENT2){
		lazycolor('%');
		fputs(globset.tabstring, stdout);
	}else{
		lazycolor(c);
#if COLOR == HTML
		const char specialchars[] = "<>&";
		if(strchr(specialchars, c)) printf("&#x%x;", c);
		else
#elif COLOR == LATEX
		const char specialchars[] = " \n\\{}<>-\"*_#&%~^$";
		char *test = strchr(specialchars, c);
		if(test){
			const char *const repl[] = {
				"\\ ",
				/* when DejaVu Sans Mono is scaled to fit Tex Gyre Pagella, for
				 * some reason, its line height is NOT scaled accordingly: */
				"\\vspace{-0.75mm}\\\\\n",
				"\\char`\\\\",
				"\\{",
				"\\}",
				"<{}",
				">{}",
				"-{}",
				"\"{}",
				"$\\ast$",
				"\\underline\\ ",
				"\\#",
				"\\&",
				"\\%",
				"\\textasciitilde ",
				"\\textasciicircum ",
				"\\$"
			};
			fputs(repl[test - specialchars], stdout);
		}
		else
#endif
		putchar(c);
		if(c == '\n') lineenum();
	}
}

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

//tweakables:
#define BUFSIZE (1 << 8)
#define HORIZON 1 /* characters we need to see forward and backward */

//configuration sanity checks:
#if 4*HORIZON > BUFSIZE
# undef RINGBUF /* configuration unsupported by ringbuf */
#endif

static char buf[BUFSIZE];
static unsigned atpos;
static char at(unsigned ind){
#ifdef RINGBUF
	return buf[(atpos + ind) % BUFSIZE];
#else
	return buf[atpos + ind];
#endif
}

static _Bool range(char test, char min, char max)
{ return (test >= min && test <= max); }

static _Bool irange(char test, char min, char max)
{ return range(test | 0x20, min, max); }

static _Bool ishex(char test)
{ return (range(test, '0', '9') || irange(test, 'a', 'f')); }

static _Bool ieq(char test, char cmp)
{ return ((test | 0x20) == cmp); }

static void iterate(unsigned pos, unsigned len){
	atpos = pos;
	while(atpos != len){
		statecode_t prevstate = curstate.state;
		//pre output state transitions
		switch(curstate.state){
		case STATE_IDENTIFIER:
			if(!isalnum(at(0)) && at(0) != '_'){
				stpop();
				continue;
			}
			break;
		case STATE_NONE:
			switch(at(0)){
			/* because shell-style comments are treated like C-preprocessor,
			do apostrophes here, neglecting them in C-preprocessor,
			so they don't ruin everything in case it was a comment */
			case '\'':
				stpush(STATE_CHR); break;
			}
			//fallthrough
		case STATE_PREPROC:
			if(isalpha(at(0)) || at(0) == '_') stpush(STATE_IDENTIFIER);
			else if(range(at(0), '1', '9')) goto decimal;
			else switch(at(0)){
			case '\"':
				stpush(STATE_STR); break;
			case '#':
				if(curstate.state != STATE_PREPROC) stpush(STATE_PREPROC);
				break;
			case '/':
				if(at(1) == '/') stpush(STATE_CPPCOMMENT);
				else if(at(1) == '*') stpush(STATE_CCOMMENT);
				break;
			case '0':
				if(ieq(at(1), 'x')){
					stpush(STATE_HEX0X);
					break;
				}else if(range(at(1), '0', '9')){
					stpush(STATE_OCT);
					break;
				}
			decimal:
				stpush(STATE_DEC);
				break;
			case '.':
				if(range(at(1), '0', '9')) goto decimal;
				break;
			case '{': enbrace(0, 0); break;
			case '(': enbrace(1, 0); break;
			case '[': enbrace(2, 0); break;
			case '}': enbrace(0, 1); break;
			case ')': enbrace(1, 1); break;
			case ']': enbrace(2, 1); break;
			}
			if(curstate.state != STATE_PREPROC) break;
		case STATE_CPPCOMMENT:
			if(at(0) == '\n'){
				stpop();
				continue;
			}else if(at(0) == '\\' && at(1) == '\n'){
				stpush(STATE_ESC);
			}
			break;
		case STATE_STR:
		case STATE_CHR:
			if(at(0) == '\\'){
				if(ieq(at(1), 'x')) stpush(STATE_HEX0X);
				else if(range(at(1), '0', '7')) stpush(STATE_OCT);
				else stpush(STATE_ESC);
			}
			break;
		case STATE_DEC:
			if(!range(at(0), '0', '9') && at(0) != '.'){
				stpop();
				continue;
			}
			break;
		case STATE_OCT:
			if(!range(at(0), '0', '7')){
				stpop();
				continue;
			}
			break;
		case STATE_HEX:
			if(!ishex(at(0))){
				stpop();
				continue;
			}
			break;
		case STATE_LINENUM:
			stpop();
			if(at(0) == '\t'){
				stpush(STATE_INDENT1);
				break;
			}
			continue;
		}

		output(at(0));

		post_output_state_transitions:
		switch(curstate.state){
		case STATE_LINENUM:
		case STATE_INDENT2:
			stpop();
			if(at(1) == '\t'){
				stpush(STATE_INDENT1);
				break;
			}
			goto post_output_state_transitions;
		case STATE_INDENT1:
			stpop();
			if(at(1) == '\t'){
				stpush(STATE_INDENT2);
				break;
			}
			goto post_output_state_transitions;
		case STATE_BRACE:
			stpop();
			goto post_output_state_transitions;
		case STATE_ESC:
			if(at(-1) == '\\') stpop();
			break;
		case STATE_CCOMMENT:
			if(at(-1) == '*' && at(0) == '/') stpop();
			break;
		case STATE_HEX0X:
			if(ieq(at(0), 'x')){
				curstate.state = STATE_HEX;
			}
			break;
		default:
			switch(prevstate){
			case STATE_STR:
				if(at(0) == '\"') stpop();
				break;
			case STATE_CHR:
				if(at(0) == '\'') stpop();
				break;
			}
			break;
		}
		atpos++;
#ifdef RINGBUF
		atpos %= BUFSIZE;
#endif
	}
}

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

#ifndef RINGBUF

/* length of iteration in buffer, constrained by the horizons of each end */
#define ITERLEN (BUFSIZE - 2*HORIZON)
#if ITERLEN >= 2*HORIZON
# define forewind memcpy
#else
# define forewind memmove
#endif

void treatfile(FILE *fp){
	perfile_init();
	signed fill = HORIZON;
	memset(buf, '\n', fill);

	for(;;){
		unsigned req = BUFSIZE - fill;
		unsigned got = fread(buf + fill, sizeof(char), req, fp);
		unsigned end = fill + got;
		unsigned stop = BUFSIZE - HORIZON;
		if(unlikely(got != req)){
			if(req - got > HORIZON){
				stop = end;
				req = got + HORIZON;
			}
			memset(buf + end, '\n', req - got);
		}
		iterate(HORIZON, stop);
		fill = end - ITERLEN;
		if(fill <= HORIZON) break;
		forewind(buf, buf + ITERLEN, fill);
	}
}

#else

#define QARTWAY (BUFSIZE/4)
#define HALFWAY (BUFSIZE/2)

void treatfile(FILE *fp){
	perfile_init();
	memset(buf + 3*QARTWAY, '\n', QARTWAY);
	unsigned fill = 0;
	unsigned pos = 0;
	unsigned halfpart = 0;

	for(;;){
		unsigned got = fread(buf + fill, sizeof(char), HALFWAY, fp);
		fill += got;
		unsigned stop = halfpart + QARTWAY;
		if(unlikely(got != HALFWAY)){
			if(got < QARTWAY){
				stop = fill;
				got = QARTWAY;
			}
			memset(buf + fill, '\n', HALFWAY - got);
		}
		iterate(pos, stop);
		if(stop == fill) break;
		pos = stop;
		halfpart ^= HALFWAY;
		fill %= BUFSIZE;
	}
}

#endif

static void precontent(){
	if(globset.doc){
#if COLOR == HTML
		puts(
			"<html><head>\n"
			"<meta http-equiv=\"content-type\" content=\"text/html;charset=utf-8\">\n"
		);
#elif COLOR == LATEX
		puts(
			"\\documentclass[a4paper,11pt]{article}\n"
			"\\usepackage[utf8]{inputenc}\n"
			"\\usepackage{courier}\n"
			"\\usepackage{color}\n"
		);
#endif

		if(globset.stylable){
# if COLOR == HTML
			puts("<style type=\"text/css\">");
# endif
			statecode_t i;
			for(i=1; i<NUM_STATES; i++){
# if COLOR == HTML
				char colstr[] = "color:#000;";
				char bgcolstr[] = "background-color:#000;";
				htmlcolorcode(colstr, 7, stylecolor[i] & 0xf, stylecolor[i] & 0xf);
				htmlcolorcode(bgcolstr, 18, stylecolor[i] >> 4, stylecolor[i] >> 4);
				printf(".%s{%s%s}\n", stylename[i], colstr, bgcolstr);
# elif COLOR == LATEX
				printf(
					"\\definecolor{%s}{RGB}{%u,%u,%u}\n", stylename[i],
					(rgb24_from_ansi4(stylecolor[i]) >>16) & 0xff,
					(rgb24_from_ansi4(stylecolor[i]) >> 8) & 0xff,
					(rgb24_from_ansi4(stylecolor[i]) >> 0) & 0xff
				);
# endif
			}
# if COLOR == HTML
			puts("</style>");
# endif
		}

#if COLOR == HTML
		puts("</head><body>");
#elif COLOR == LATEX
		puts("\\begin{document}");
#endif

	}//doc

#if COLOR == HTML
	puts(
		"<" HTMLELEMENT " style=\"color:#fff;"
		"background-color:#000;line-height:100%;\">"
	);
#elif COLOR == LATEX
	puts("{\\ttfamily \\raggedright \\small");
#endif
	linenum = 1;
	lineenum();
}

static void postcontent(){
#if COLOR == HTML
	puts("</" HTMLELEMENT ">");
	if(globset.doc) puts("</body></html>");
#elif COLOR == LATEX
	puts("}\n\\normalfont \\normalsize");
	if(globset.doc) puts("\\end{document}");
#else
	lazycolor('%');
	putchar('\n');
#endif
}

static void help(
	const struct narg_optspec *opts,
	const char ***ans, unsigned optc
){
	unsigned pos;
	flockfile(stdout);
	
	pos=0;
	narg_indentputs_unlocked(
		stdout, &pos, 0, globset.width,
		"colorcat – the syntax highlighting cat\n"
		"\n\tUsage:\n"
		"colorcat options and filenames\n"
		"\nFiles get printed concatenated in the specified order,"
		" with generic syntax highlighing that works for many filetypes."
		"\nThe empty filename \'\' is standard input."
		"\nOptions may intermingle filenames. Their order has no effect,"
		" except that later options take precedence over previous"
		" options should they contradict.\n"
		"\n\tOptions:"
	);
	fputc_unlocked('\n', stdout);
	
	narg_printopt_unlocked(stdout, ans, opts, optc, 2, globset.width);
	
	funlockfile(stdout);
}

static _Bool valid_formatstring(const char *str, const char *whitelist){
	char *pos = strchr(str, '%');
	if(pos){
		pos++;
		if(strchr(pos, '%')) return 0;
		while(*pos >= '0' && *pos <= '9') pos++;
		if(!strchr(whitelist, *pos)) return 0;
	}
	char buf[21];
	snprintf(buf, 21, str, 0);
	unsigned len = strlen(buf);
	if(len > 19) return 0;
	globset.numformat = str;
	globset.numwidth = len;
	return ~0;
}

static unsigned main_init(int argc, char **argv){
	setlocale(LC_ALL, "");
	const_init();
	globset.width = narg_terminalwidth(stdout);
	globset.numformat = "%4u";
	globset.tabstring = "⎟   ";
	char *preset_width = "width of your terminal";
	
	enum valg{
		OPT_HLP,
		OPT_DOC,
		OPT_STY,
		OPT_NUM,
		OPT_TAB,
		OPT_WDT,
		OPT_END,
		NUM_OPTS
	};
	static const struct narg_optspec opts[NUM_OPTS] = {
		{"h", "help",     NULL, "Show help"},
		{"d", "doc",      NULL, "Ouput standalone document (with headers)"},
		{"s", "stylable", NULL, "Ouput labeled markup instead of hardcoded colors. When in document mode, also define the colors (in headers)"},
		{"n", "num",      "=FORMAT", "Set format string of line numbering"},
		{"t", "tab",      "=STRING", "Set the string that replaces leading tabs"},
		{"w", "width",    "=NUMBER", "Set word wrap width"},
		{NULL, "",        &narg_metavar.ignore_rest, "Treat subsequent arguments as filenames"}
	};
	const char *nans_num[1] = {globset.numformat};
	const char *nans_tab[1] = {globset.tabstring};
	const char *nans_wdt[1] = {preset_width};
	const char **ans[NUM_OPTS] = {
		NULL,
		NULL,
		NULL,
		nans_num,
		nans_tab,
		nans_wdt,
		NULL
	};
	
	struct narg_result res = narg_argparse(ans, argv, opts, NUM_OPTS, 2, ~0);
	
	globset.doc      = ans[OPT_DOC] ? ~0 : 0;
	globset.stylable = ans[OPT_STY] ? ~0 : 0;
	
	if(res.err){
		const char *errstr =
			(res.err == NARG_ENOSUCHOPTION)    ? "No such option":
			(res.err == NARG_EUNEXPECTEDPARAM) ? "Unexpected parameter":
			(res.err == NARG_EMISSINGPARAM)    ? "Missing parameter":
			"";
		fprintf(stderr, "Argument %u: %s: %s.\n", res.arg, argv[res.arg], errstr);
		exit(1);
	}
	/*ans[OPT_NUM]*/{
		const char *whitelist = "uoxXp";
		if(!valid_formatstring(ans[OPT_NUM][0], whitelist)){
			fprintf(
				stderr, "%s: Only conversion specifiers \"%s\" supported.\n",
				ans[OPT_NUM][0], whitelist
			);
			exit(1);
		}
	}
	if(ans[OPT_TAB][0] != globset.tabstring){
		globset.tabstring = ans[OPT_TAB][0];
	}
	if(ans[OPT_WDT][0] != preset_width){
		globset.width = strtoul(ans[OPT_WDT][0], NULL, 0);
	}
	if(ans[OPT_HLP]){
		ans[OPT_HLP] = NULL;
		help(opts, ans, NUM_OPTS);
		exit(0);
	}
	return res.arg;
}

int main(int argc, char *argv[]){
	unsigned inc = main_init(argc, argv);
	argc -= inc;
	argv += inc;
	
	precontent();
	for(; argc--; argv++){
		if(**argv == '\0'){
			treatfile(stdin);
			continue;
		}
		FILE *fp = fopen(*argv, "r");
		if(fp == NULL){
			perror(*argv);
			continue;
		}
		treatfile(fp);
		fclose(fp);
	}
	postcontent();
	return 0;
}
