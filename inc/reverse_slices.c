
static void swap_partitions(const char **a, const char **b, unsigned len)
{
	while (len--) {
		const char *tmp = a[len];
		a[len] = b[len];
		b[len] = tmp;
	}
}

static void reverse_slices(const char **base, unsigned pivot, unsigned len)
{
	for (;;) {
		unsigned len_shortest = (pivot < len-pivot) ? pivot : len-pivot;
		if (len_shortest == 0) break;
		swap_partitions(base, base+pivot, len_shortest);
		if (pivot != len_shortest) {
			pivot -= len_shortest;
		}
		base += len_shortest;
		len -= len_shortest;
	}
}

#ifdef TEST
#include "../testapi/testability.h"

static void test_reverse_slices(int *status, const char **base, unsigned pivot, unsigned len, const char **expect)
{
	reverse_slices(base, pivot, len);
	compare_slices(status, base, expect, len);
}

int main() {
	int status = 0;
	test_reverse_slices(&status, (const char *[]){0}, 0, 0, (const char *[]){0});
	test_reverse_slices(&status, (const char *[]){"x"}, 0, 1, (const char *[]){"x"});
	test_reverse_slices(&status, (const char *[]){"x"}, 1, 1, (const char *[]){"x"});
	test_reverse_slices(&status, (const char *[]){"a","b"}, 0, 2, (const char *[]){"a","b"});
	test_reverse_slices(&status, (const char *[]){"a","x"}, 1, 2, (const char *[]){"x","a"});
	test_reverse_slices(&status, (const char *[]){"x","y"}, 2, 2, (const char *[]){"x","y"});
	test_reverse_slices(&status, (const char *[]){"a","x","y"}, 1, 3, (const char *[]){"x","y","a"});
	test_reverse_slices(&status, (const char *[]){"a","b","x"}, 2, 3, (const char *[]){"x","a","b"});
	test_reverse_slices(&status, (const char *[]){"a","b","x","y"}, 2, 4, (const char *[]){"x","y","a","b"});
	test_reverse_slices(&status, (const char *[]){"x","y","z","æ","ø"}, 0, 5, (const char *[]){"x","y","z","æ","ø"});
	test_reverse_slices(&status, (const char *[]){"a","x","y","z","æ"}, 1, 5, (const char *[]){"x","y","z","æ","a"});
	test_reverse_slices(&status, (const char *[]){"a","b","x","y","z"}, 2, 5, (const char *[]){"x","y","z","a","b"});
	test_reverse_slices(&status, (const char *[]){"a","b","c","x","y"}, 3, 5, (const char *[]){"x","y","a","b","c"});
	test_reverse_slices(&status, (const char *[]){"a","b","c","d","x"}, 4, 5, (const char *[]){"x","a","b","c","d"});
	test_reverse_slices(&status, (const char *[]){"a","b","c","d","e"}, 5, 5, (const char *[]){"a","b","c","d","e"});
	return status;
}
#endif //TEST
