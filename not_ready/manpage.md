GETOPT_UNITED 3
===============

NAME
----

getopt_united — non-iterative getter & prettyprinter of command-line options.

SYNOPSIS
--------

 `#include <getopt_united.h>`

 `int getopt_united (
     char const *`   _argv_`[],
     struct optspec` _opts_`[],
     unsigned`       _flags_`
 );`

 `void printopt_united (
     const struct optspec` _opts_`[],
     const struct opthelp` _help_`[],
     unsigned`             _flags_`
 );`

 `struct optspec {
     uint32_t`    _nparams_`;
     uint32_t`    _shortopt_`;
     const char *`_longopt_`;
     const char *`_ans_`;
 };`

 `struct opthelp {
     const char *`_templ_`;
     const char *`_descr_`;
 };`

DESCRIPTION
-----------

`getopt_united()` finds options among arguments in _argv_[],
looks up each option in _shortopt_ or _longopt_ to find the element of _opts_
to which _ans_ is set to non-NULL.


The number of parameters is given by _nparams_.

Options are those elements of _argv_[] that begin with a dash '-',
comprises more characters than a sole dash,
and are not a parameter of the previous option.
If the option matches a _shortopt_ or _longopt_ in the _opts_ array,
then the following _nparams_

ERRORS
------

* Not an option
* Missing parameter
* Unexpected parameter

ENVIRONMENT
-----------

`LC_CTYPE`
  the character set specified by the LC_CTYPE category of the current locale is used

`LANG`
  If LC

BUGS
----

Utf-8 is the only charset supported.

RATIONALE
---------

1 Less boilerplate: A non-iterative option getter takes less effort to use — call it once, and the encountered options will have their parameters filled in.
2 Write options once: 

AUTHOR
------

Andreas Nordal <andreas.nordal@gmail.com>

SEE ALSO
--------

[Dupedit](http://dupedit.com),
getopt(3)
