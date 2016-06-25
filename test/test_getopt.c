#include "../narg.h"
#include <stdlib.h> //exit

void succeed_nothing(){
	static char a[] = "pluto";
	static char b[] = "nix";
	static char c[] = "hydra";
	char *argv[] = {
		a,
		b,
		c,
		NULL
	};
	
	const struct narg_optspec optv[1] = {
		{"ø", "møndarn", "1", "Je ska på prooogym"}
	};
	const char *broiler[] = {"hokksund"};
	const char **ans[1] = {
		broiler
	};
	
	unsigned togglebox;
	for (togglebox=0; togglebox!=8; togglebox++){
		unsigned dashes = 1 + (togglebox >> 2);
		unsigned argc = 1 + ((togglebox >> 1) & 1);
		unsigned optc = togglebox & 1;
		
		struct narg_result res = narg_argparse(
			ans, argv, optv, optc, dashes, ~0
		);
		if(res.err == 0 && res.arg == 1)
		if(argv[0] == a && argv[1] == b && argv[2] == c)
		if(ans[0] == broiler)
			continue;
		
		printf("FAIL @ "__FILE__ " %s(argc=%u, optc=%u, dashes=%u)\n", __func__, argc, optc, dashes);
		exit(1);
	}
}

void succeed_dash0_parm0(){
	const char *deFault[] = {"urørt", NULL};
	const char **ans[7];
	struct narg_optspec optv[7] = {
		{NULL, " ", 0},
		{NULL, "à⎟łeteik\n-\e\rskapt–lïké ←↓→↑", 0},
		{NULL, "æ\u0307", 0},
		{NULL, "", 0},
		{NULL, "-", 0},
		{NULL, "--", 0},
		{NULL, "---", 0}
	};
	char *argv[3];
	char *vardash[3*2] = {
		"---" "felle",
		"---" "felle",
		"---" " ",
		"---" "à⎟łeteik\n-\e\rskapt–lïké ←↓→↑",
		"---" "æ\u0307",
		"---"
	};
	
	unsigned togglebox;
	for (togglebox=0; togglebox!=16; togglebox++){
		unsigned i;
		for (i=0; i<7; i++){
			ans[i] = deFault;
		}
		
		if(togglebox == 8) for(i=0; i!=7; i++){
			optv[i].shortopt = optv[i].longopt;
			optv[i].longopt = NULL;
		}
		unsigned dashcount = (togglebox >> 1) & 3;
		unsigned oddeven = togglebox & 1;
		
		for(i=0; i!=3; i++){
			argv[i] = vardash[(i*2) | oddeven] + 3-dashcount;
		}
		struct narg_result res = narg_argparse(
			ans, argv, optv, 7, 3, ~0
		);
		
		if(res.err == 0 && res.arg == 3)
		if(argv[0] == vardash[oddeven] + 3-dashcount)
		{
			if(oddeven == 0){
				if(ans[0][0] == vardash[2] + 3)
				if(ans[1] == deFault)
				if(ans[2][0] == vardash[4] + 3)
				continue;
			}else{
				if(ans[0] == deFault)
				if(ans[1][0] == vardash[3] + 3)
				if(ans[2] == deFault)
				if(ans[3 + dashcount][0] == vardash[5] + 3-dashcount)
				continue;
			}
		}
		printf("FAIL @ "__FILE__ " %s(togglebox=%u) → err=%u, arg=%u\n", __func__, togglebox, res.err, res.arg);
		exit(1);
	}
}

int main(){
	succeed_nothing();
	succeed_dash0_parm0();
	return 0;
}
