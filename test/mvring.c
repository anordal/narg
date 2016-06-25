#define _GNU_SOURCE //fputs_unlocked
#include <unistd.h> //close
#include <sys/types.h> //open
#include <sys/stat.h> //open
#include <fcntl.h> //open
#include <stdlib.h>
#include <errno.h> //strerror
#include <stdio.h> //fprintf
#include <string.h> //strcmp
#include <locale.h>

#include "../narg.h"

static void help(const struct narg_optspec opts[], const char** ans[], unsigned optc){
	unsigned width = narg_terminalwidth(stdout), pos;
	flockfile(stdout);
	
	fputs_unlocked("mvchain:", stdout); pos=8;
	narg_indentputs_unlocked(
		stdout, &pos, 9, width,
		"move files like on a conveyorbelt\n"
		"old.txt → existing.txt → new.txt"
	);
	fputs_unlocked("\n\n", stdout);
	
	fputs_unlocked("mvring:", stdout); pos=7;
	narg_indentputs_unlocked(
		stdout, &pos, 8, width,
		"move files circularly (e.g. swap filenames)\n"
		"a.txt → b.txt → c.txt\n"
		"   `← ← ← ← ← ← ←´"
	);
	fputs_unlocked("\n\n", stdout);
	
	pos=0;
	narg_indentputs_unlocked(
		stdout, &pos, 0, width,
		"\tUsage:\n"
		"mvring  existing_file1 existing_file2 ...\n"
		"mvchain old_file [existing_files] new_file\n"
		"\n\tDescription:\n"
		"Mvchain moves the last of existing_files to new_file, "
		"the next-last to the last, and so on.\n"
		"Mvring invents a random new_file name, "
		"does mvchain with this as last argument, "
		"and moves new_file over the moved-away first file.\n"
		"Unless one of the --ring and --chain options are given, "
		"the invocation name determines the action.\n"
		"\n\tOptions:"
	);
	fputc_unlocked('\n', stdout);
	
	narg_printopt_unlocked(stdout, ans, opts, optc, 2, width);
	
	pos=0;
	narg_indentputs_unlocked(
		stdout, &pos, 0, width,
		"\n\tExamples:\n"
		"mvchain a b c #equivalent to mv b c && mv a b\n"
		"mvring  a b   #equivalent to mv b c && mv a b && mv c a\n"
	);
	
	funlockfile(stdout);
}

static int rename_noclobber(char *a, char *b){
	struct stat info;
	int fail = stat(a, &info);
	if(fail){
		perror(a);
		return fail;
	}
	int fd = -1;
	if(S_ISREG(info.st_mode)){
		fd = open(b, O_CREAT|O_EXCL, info.st_mode);
		if(fd == -1){
			perror(b);
			return -1;
		}
		//We exclusively own the target
	}

	fail = rename(a, b);
	if(fail){
		fprintf(stderr, "%s → %s: %s\n", a, b, strerror(errno));
		if(fd != -1){
			unlink(b);
		}
	}
	
	if(fd != -1) close(fd);
	return fail;
}

static int chain(unsigned count, char *names[]){
	int fail = 0;
	while(--count){
		fail = rename_noclobber(names[count-1], names[count]);
		if(fail) break;
	}
	return fail;
}

#define CHAR_DIRSEP '/'
static const char *basename_const(const char *s){
	const char *i = s + strlen(s);
	while(i != s && *--i == CHAR_DIRSEP);
	while(i != s && *--i != CHAR_DIRSEP);
	if(*i == CHAR_DIRSEP) ++i;
	return i;
}

static int ring(unsigned count, char *names[]){
	static const char tmpname[] = ".mvring.tmp";
	const char *basename = basename_const(names[0]);
	unsigned before_basename = basename - names[0];
	char tmppath[before_basename + sizeof(tmpname)]; //VLA
	memcpy(tmppath, names[0], before_basename);
	memcpy(tmppath + before_basename, tmpname, sizeof(tmpname));

	int fail = rename_noclobber(names[count-1], tmppath);
	if(fail) return fail;
	fail = chain(count, names);
	if(fail) return fail;
	fail = rename_noclobber(tmppath, names[0]);
	return fail;
}

static _Bool hasbase(const char *path, const char *base){
	const char *pos = strrchr(path, CHAR_DIRSEP);
	if(pos) pos++;
	else pos = path;
	return (0 == strcmp(pos, base));
}

static void snu(char **foran, char **bak){
	while(foran < bak){
		register char *temp = *foran;
		*foran = *bak;
		*bak = temp;
		foran++; bak--;
	}
}

int main(int argc, char *argv[]){
	setlocale(LC_ALL, "");
	enum hvafornoe{
		OPT_HLP,
		OPT_RNG,
		OPT_CHN,
		OPT_REV,
		OPT_END,
		NUM_OPTS
	};
	static const struct narg_optspec opts[NUM_OPTS] = {
		{"h", "help",    NULL, "Show help"},
		{"o", "ring",    NULL, "Move names circularly"},
		{"c", "chain",   NULL, "Move names like on a conveyor belt"},
		{"r", "reverse", NULL, "Move in reverse direction"},
		{NULL, "",       &narg_metavar.ignore_rest, "Treat subsequent arguments as filenames"}
	};
	const char **ans[NUM_OPTS] = {0};
	const char **arg0 = (const char**)argv+0;
	{
		struct narg_result res = narg_argparse(ans, argv, opts, NUM_OPTS, 2, ~0);
		if(res.err){
			const char *errstr =
				(res.err == NARG_ENOSUCHOPTION)    ? "No such option":
				(res.err == NARG_EMISSINGPARAM)    ? "Missing parameter":
				(res.err == NARG_EUNEXPECTEDPARAM) ? "Unexpected parameter":
				"";
			fprintf(stderr, "Argument %u: %s: %s.\n", res.arg, argv[res.arg], errstr);
			return 1;
		}
		argc -= res.arg;
		argv += res.arg;
	}
	if(ans[OPT_HLP]){
		ans[OPT_HLP] = NULL;
		help(opts, ans, NUM_OPTS);
		return 0;
	}
	if(argc < 2){
		return 0;
	}
	
	if(ans[OPT_CHN] && ans[OPT_RNG]){
		ans[OPT_CHN] = NULL;
		ans[OPT_RNG] = NULL;
	}
	if(ans[OPT_CHN] == ans[OPT_RNG]){
		ans[hasbase(*arg0, "mvring") ? OPT_RNG : OPT_CHN] = arg0;
	}
	if(ans[OPT_REV]) snu(argv, argv+argc-1);
	
	int fail;
	if(ans[OPT_RNG]){
		fail = ring(argc, argv);
	}else{
		fail = chain(argc, argv);
	}
	return fail;
}
