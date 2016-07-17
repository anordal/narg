Narg
----

Narg is an option parser/printer for command line programs in C/C++.

Inspired by python's legendary `argparse` function, and named after its `nargs` argument, Narg is a multiarg option parser, that lets you specify options once (for both parsing and printing) in a structured way (table of structs).

To make it simple, the parsing only consists of identifying options and their parameters; you have to parse the strings yourself when they represent other things than strings.

The printing is supposed to be better than humanly formatted, by adjusting to the width of the terminal and recreating the indentation for overflowing text. Internationalization of option descriptions happens at printing time.

## Multiarg

You can have options that take exactly N parameters. For example, `docker` has an option, `-v`, that takes one parameter, but the parameter is really two paths separated by colon:

    docker -v from-path:to-path

As you can imagine, that's going to blow up if any of those paths contain a colon! Had they used Narg however, they could have avoided that ugly string concatenation:

    docker -v from-path to-path

Furthermore, options can be repeated, allowing *multiples* of exactly N arguments.

## Other features
* No hidden state
* Good test coverage
* No memory allocation
* Customizable number of leading long-option dashes (most programs use 2, but `ffmpeg -codecs` would be 1 and `dd bs=1` would be 0).

## TODO (short term)
* manpage, with example code
* checks for custom policies on code and exported symbols
* Complete test coverage (by lines)

## Goals (long term)
* Complete UTF-8 compatibility (TODO: multi-width characters)
