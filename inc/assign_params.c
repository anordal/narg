#include "../api/narg.h"

static void reverse_slices(const char **base, unsigned pivot, unsigned len);

static void assign_params(
	const char **argv,
	struct narg_paramret *retv,
	unsigned optc,
	unsigned o,
	unsigned optcount,
	unsigned paramcount,
	const char **paramvec)
{
	// Overwrite nothing or default
	if(
		retv[o].paramc == 0 ||
		retv[o].paramv < argv ||
		retv[o].paramv > paramvec
	){
		retv[o].paramv = paramvec;
		retv[o].paramc = paramcount;
		return;
	}
	// Move to the end and append
	// Entails moving anything inbetween in the opposite direction
	if (retv[o].paramv + retv[o].paramc != paramvec - optcount){
		for(unsigned i=0; i<optc; ++i){
			if(retv[i].paramv > retv[o].paramv && retv[i].paramv < paramvec){
				retv[i].paramv -= retv[o].paramc;
			}
		}
	}
	reverse_slices(retv[o].paramv, retv[o].paramc, paramvec - retv[o].paramv);
	retv[o].paramv = paramvec - retv[o].paramc;
	retv[o].paramc += paramcount;
}

#ifdef TEST
#undef TEST
#include "reverse_slices.c"
#include "macros_test.h"
#include <stdio.h>

static void expect_paramret(int *status, const struct narg_paramret *actual, unsigned expectedlen, const char **expected)
{
	if(actual->paramc != expectedlen){
		fprintf(stderr, "%d != %d\n", actual->paramc, expectedlen);
		*status = 1;
		return;
	}
	for(unsigned i=0; i!=expectedlen; ++i) {
		if (actual->paramv[i] != expected[i]) {
			fprintf(stderr, "%s != %s\n", actual->paramv[i], expected[i]);
			*status = 1;
		}
	}
}

static void nothing_vs_default(int *status){
	const char *argv[] = {
		"Kaptein",
		"Sabeltann",
		"lukter",
		"gull",
		NULL
	};
	struct narg_paramret retv[] = {
		{0, NULL},
		{2, (const char*[]){"Kaptein","Sabeltann"}}
	};
	expect_paramret(status, retv+0, 0, NULL);
	expect_paramret(status, retv+1, 2, (const char*[]){"Kaptein","Sabeltann"});

	assign_params(argv, retv, ARRAY_SIZE(retv), 0, 0, 2, argv+0);

	expect_paramret(status, retv+0, 2, (const char*[]){"Kaptein","Sabeltann"});
	expect_paramret(status, retv+1, 2, (const char*[]){"Kaptein","Sabeltann"});

	assign_params(argv, retv, ARRAY_SIZE(retv), 0, 0, 2, argv+2); // append
	assign_params(argv, retv, ARRAY_SIZE(retv), 1, 0, 2, argv+2); // overwrite

	expect_paramret(status, retv+0, 4, (const char*[]){"Kaptein","Sabeltann","lukter","gull"});
	expect_paramret(status, retv+1, 2, (const char*[]){"lukter","gull"});
}

static void append_opt_x_param(int *status){
	const char *argv[] = {
		//--0+0
		//--0+1
		"ole",
		//--0+2
		"dole",
		"doffen",
		"--1+0",
		"--1+1",
		"kasper",
		"--1+2",
		"jesper",
		"jonatan",
		"--2",
		"+0",
		"--2",
		"+1",
		"doffen",
		"--2",
		"+2",
		"har",
		"dævva",
		NULL
	};
	struct narg_paramret retv[] = {
		{0, NULL}
	};
	expect_paramret(status, retv+0, 0, NULL);

	assign_params(argv, retv, ARRAY_SIZE(retv), 0, 0, 0, argv+0);
	assign_params(argv, retv, ARRAY_SIZE(retv), 0, 0, 1, argv+0);
	assign_params(argv, retv, ARRAY_SIZE(retv), 0, 0, 2, argv+1);

	expect_paramret(status, retv+0, 3, (const char*[]){"ole","dole","doffen"});

	assign_params(argv, retv, ARRAY_SIZE(retv), 0, 1, 0, argv+4);
	assign_params(argv, retv, ARRAY_SIZE(retv), 0, 1, 1, argv+5);
	assign_params(argv, retv, ARRAY_SIZE(retv), 0, 1, 2, argv+7);

	expect_paramret(status, retv+0, 6, (const char*[]){"ole","dole","doffen","kasper","jesper","jonatan"});

	assign_params(argv, retv, ARRAY_SIZE(retv), 0, 2, 0, argv+11);
	assign_params(argv, retv, ARRAY_SIZE(retv), 0, 2, 1, argv+13);
	assign_params(argv, retv, ARRAY_SIZE(retv), 0, 2, 2, argv+16);

	expect_paramret(status, retv+0, 9, (const char*[]){"ole","dole","doffen","kasper","jesper","jonatan","doffen","har","dævva"});
}

static void reorder_noopt(int *status){
	const char *argv[] = {
		"Kaptein",
		"Sabeltann",
		"Captain",
		"Slow",
		"lukter",
		"gull",
		"smells",
		"petrol",
		NULL
	};
	struct narg_paramret retv[] = {
		{0, NULL},
		{0, NULL}
	};
	assign_params(argv, retv, ARRAY_SIZE(retv), 0, 0, 2, argv+0);
	assign_params(argv, retv, ARRAY_SIZE(retv), 1, 0, 2, argv+2);
	assign_params(argv, retv, ARRAY_SIZE(retv), 0, 0, 2, argv+4);
	assign_params(argv, retv, ARRAY_SIZE(retv), 1, 0, 2, argv+6);

	expect_paramret(status, retv+0, 4, (const char*[]){"Kaptein","Sabeltann","lukter","gull"});
	expect_paramret(status, retv+1, 4, (const char*[]){"Captain","Slow","smells","petrol"});
}

static void reorder_yesopt(int *status){
	const char *argv[] = {
		"--nb_NO",
		"Kaptein",
		"Sabeltann",
		"--en_GB",
		"Captain",
		"Slow",
		"--nb_NO",
		"lukter",
		"gull",
		"--en_GB",
		"smells",
		"petrol",
		NULL
	};
	struct narg_paramret retv[] = {
		{0, NULL},
		{0, NULL}
	};
	assign_params(argv, retv, ARRAY_SIZE(retv), 0, 1, 2, argv+1);
	assign_params(argv, retv, ARRAY_SIZE(retv), 1, 1, 2, argv+4);
	assign_params(argv, retv, ARRAY_SIZE(retv), 0, 1, 2, argv+7);
	assign_params(argv, retv, ARRAY_SIZE(retv), 1, 1, 2, argv+10);

	expect_paramret(status, retv+0, 4, (const char*[]){"Kaptein","Sabeltann","lukter","gull"});
	expect_paramret(status, retv+1, 4, (const char*[]){"Captain","Slow","smells","petrol"});
}

int main(){
	int status = 0;
	nothing_vs_default(&status);
	append_opt_x_param(&status);
	reorder_noopt(&status);
	reorder_yesopt(&status);
	return status;
}

#endif //TEST
