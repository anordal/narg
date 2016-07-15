project('narg', 'c', default_options : 'c_std=c99', license : 'MPL 2.0')

narg_sources = [
	'api/argparse.c',
	'api/printopt.c',
	'api/utf8len.c',
	'api/wordcount.c',
]

version = '0.3.0'
narg = library('narg', sources : narg_sources, install : true, version : version)

unittests = [
	'inc/str.c',
	'inc/reverse_slices.c',
	'inc/assign_params.c',
	'api/utf8len.c',
	'api/wordcount.c',
	'api/argparse.c',
	'testapi/testability.c',
]

testapi = static_library('testability', sources : ['testapi/testability.c'])

foreach u : unittests
	dir = u.split('/').get(0)
	name = 'test_' + u.split('/').get(1).split('.').get(0)
	libs = [ testapi ]
	if dir == 'api'
		libs += [narg]
	endif
	exe = executable(name, u, c_args : '-DTEST', link_with : libs)
	test(name, exe)
endforeach

install_headers('api/narg.h')
pkg_mod = import('pkgconfig')
pkg_mod.generate(
	libraries : narg,
	version : version,
	name : 'libnarg',
	filebase : 'narg',
	description : 'A commandline option parser library.'
)
