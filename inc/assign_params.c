#include "../api/narg.h"

static void reverse_slices(const char **base, unsigned pivot, unsigned len);

static void slide_paramrange(
	struct narg_paramret *retv, unsigned optc,
	const char **from, const char **to, signed amount)
{
	if (from != to){
		for(unsigned i=0; i<optc; ++i){
			if(retv[i].paramv >= from && retv[i].paramv < to){
				retv[i].paramv += amount;
			}
		}
	}
}

static void assign_params(
	struct narg_paramret *assign,
	const char **paramvec,
	unsigned paramcount,
	unsigned optcount,
	struct narg_paramret *posargs,
	struct narg_paramret *retv,
	unsigned optc,
	const char **argv)
{
	// Overwrite nothing or default
	if(
		assign->paramc == 0 ||
		assign->paramv < argv ||
		assign->paramv > paramvec
	){
		assign->paramv = paramvec;
		assign->paramc = paramcount;
		return;
	}
	// Move to the end and append
	// Entails moving anything inbetween in the opposite direction
	slide_paramrange(posargs, 1, assign->paramv + assign->paramc, paramvec - optcount, -assign->paramc);
	slide_paramrange(retv, optc, assign->paramv + assign->paramc, paramvec - optcount, -assign->paramc);
	reverse_slices(assign->paramv, assign->paramc, paramvec - assign->paramv);
	assign->paramv = paramvec - assign->paramc;
	assign->paramc += paramcount;
}

#ifdef TEST
#undef TEST
#include "reverse_slices.c"
#include "../testapi/testability.h"

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
	struct narg_paramret posargs = {0};
	expect_paramret(status, retv+0, 0, NULL);
	expect_paramret(status, retv+1, 2, (const char*[]){"Kaptein","Sabeltann"});

	assign_params(retv+0, argv+0, 2, 0, &posargs, retv, ARRAY_SIZE(retv), argv);

	expect_paramret(status, retv+0, 2, (const char*[]){"Kaptein","Sabeltann"});
	expect_paramret(status, retv+1, 2, (const char*[]){"Kaptein","Sabeltann"});

	assign_params(retv+0, argv+2, 2, 0, &posargs, retv, ARRAY_SIZE(retv), argv); // append
	assign_params(retv+1, argv+2, 2, 0, &posargs, retv, ARRAY_SIZE(retv), argv); // overwrite

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
	struct narg_paramret posargs = {0};
	expect_paramret(status, retv+0, 0, NULL);

	assign_params(retv+0, argv+0, 0, 0, &posargs, retv, ARRAY_SIZE(retv), argv);
	assign_params(retv+0, argv+0, 1, 0, &posargs, retv, ARRAY_SIZE(retv), argv);
	assign_params(retv+0, argv+1, 2, 0, &posargs, retv, ARRAY_SIZE(retv), argv);

	expect_paramret(status, retv+0, 3, (const char*[]){"ole","dole","doffen"});

	assign_params(retv+0, argv+4, 0, 1, &posargs, retv, ARRAY_SIZE(retv), argv);
	assign_params(retv+0, argv+5, 1, 1, &posargs, retv, ARRAY_SIZE(retv), argv);
	assign_params(retv+0, argv+7, 2, 1, &posargs, retv, ARRAY_SIZE(retv), argv);

	expect_paramret(status, retv+0, 6, (const char*[]){"ole","dole","doffen","kasper","jesper","jonatan"});

	assign_params(retv+0, argv+11, 0, 2, &posargs, retv, ARRAY_SIZE(retv), argv);
	assign_params(retv+0, argv+13, 1, 2, &posargs, retv, ARRAY_SIZE(retv), argv);
	assign_params(retv+0, argv+16, 2, 2, &posargs, retv, ARRAY_SIZE(retv), argv);

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
	struct narg_paramret posargs = {0};
	assign_params(retv+0, argv+0, 2, 0, &posargs, retv, ARRAY_SIZE(retv), argv);
	assign_params(retv+1, argv+2, 2, 0, &posargs, retv, ARRAY_SIZE(retv), argv);
	assign_params(retv+0, argv+4, 2, 0, &posargs, retv, ARRAY_SIZE(retv), argv);
	assign_params(retv+1, argv+6, 2, 0, &posargs, retv, ARRAY_SIZE(retv), argv);

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
	struct narg_paramret posargs = {0};
	assign_params(retv+0, argv+1, 2, 1, &posargs, retv, ARRAY_SIZE(retv), argv);
	assign_params(retv+1, argv+4, 2, 1, &posargs, retv, ARRAY_SIZE(retv), argv);
	assign_params(retv+0, argv+7, 2, 1, &posargs, retv, ARRAY_SIZE(retv), argv);
	assign_params(retv+1, argv+10, 2, 1, &posargs, retv, ARRAY_SIZE(retv), argv);

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
