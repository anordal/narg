#include <wchar.h>
#include <stdio.h>

struct optdesc{       // example:
	wchar_t shortopt; // 'o'
	char *longopt;    // "output"
	char *argtempl;   // "=FILENAME"
	char *desc;       // "Set filename of output."
	char *ans;        // "default.out"
};

unsigned terminalwidth(void);

void indentputs_unlocked(
	FILE *fp, unsigned *posPtr,
	unsigned indent, unsigned width, char *str
);

void printopt_unlocked(FILE *fp, struct optdesc flag[], unsigned width);
unsigned getopt_ng(unsigned argc,char *argv[],struct optdesc flag[]);
