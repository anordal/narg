project('narg', 'c',
	default_options : ['c_std=c99', 'warning_level=3'],
	license : 'MPL 2.0'
)

version = '1.0.0'

narg_sources = [
	'api/findopt.c',
	'api/printopt.c',
	'api/utf8len.c',
]

unittests = narg_sources + [
	'inc/assign_params.c',
	'inc/reverse_slices.c',
	'inc/str.c',
	'inc/wordcount.c',
	'testapi/testability.c',
]

if meson.is_subproject()
	narg = static_library('narg', sources : narg_sources)
	narg_dep = declare_dependency(
		link_with: narg,
		include_directories: include_directories('api')
	)
else
	narg = library('narg', sources : narg_sources, install : true, version : version)
	install_headers('api/narg.h')
	pkg_mod = import('pkgconfig')
	pkg_mod.generate(
		libraries : narg,
		version : version,
		name : 'libnarg',
		filebase : 'narg',
		description : 'A commandline option parser library.'
	)
endif

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
