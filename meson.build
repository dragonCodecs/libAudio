# SPDX-License-Identifier: BSD-3-Clause
project(
	'libAudio',
	'cpp',
	default_options: [
		'cpp_std=c++17',
		'warning_level=3',
		'buildtype=release',
		'strip=true',
		'b_ndebug=if-release',
		'b_lto=true'
	],
	version: '0.5.2',
	meson_version: '>= 0.53',
	subproject_dir: 'deps'
)

cxx = meson.get_compiler('cpp')
debug = get_option('debug')
substrate = subproject(
	'substrate',
	required: true,
	version: '>=0.0.1',
	default_options: [
		'build_tests=false'
	]
).get_variable(
	'substrate_dep'
).partial_dependency(
	compile_args: true,
	includes: true
)
substrateDir = meson.current_source_dir() / 'deps/substrate'

add_project_arguments(
	cxx.get_supported_arguments(
		'-Wvla', '-Wimplicit-fallthrough', '-Wno-attributes'
	), language: 'cpp'
)

if target_machine.system() == 'windows'
	add_project_arguments(
		'-D_WINDOWS',
		'-wd5030',
		language: 'cpp'
	)
endif

if cxx.get_id() == 'msvc'
	add_project_arguments(
		'-D_CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES=1',
		'-D_CRT_NONSTDC_NO_WARNINGS=1',
		'-D_CRT_SECURE_NO_WARNINGS',
		language: 'cpp'
	)
elif (cxx.get_id() == 'gcc' or cxx.get_id() == 'clang') and not debug
	add_project_arguments(
		'-D_FORTIFY_SOURCE=2',
		language: 'cpp'
	)
endif

crunchMake = find_program('crunchMake', required: false)

subdir('libAudio')
if not meson.is_subproject()
	subdir('player')
	subdir('transcoder')
endif
subdir('test', if_found: crunchMake)
if not meson.is_subproject()
	subdir('harness')
	subdir('bindings')
	if get_option('spectrometer')
		subdir('spectrometer')
	endif
endif

runClangTidy = find_program('runClangTidy.py')
run_target(
        'clang-tidy',
        command: [runClangTidy, '-s', meson.current_source_dir(), '-p', meson.current_build_dir()]
)
