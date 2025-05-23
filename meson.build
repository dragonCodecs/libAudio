# SPDX-License-Identifier: BSD-3-Clause
# SPDX-FileCopyrightText: 2018-2023 Rachel Mant <git@dragonmux.network>
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
	version: '0.6.0',
	meson_version: '>= 0.56',
	subproject_dir: 'deps',
	license: 'BSD-3-Clause'
)

cxx = meson.get_compiler('cpp')
python = import('python')
python = python.find_installation('python3', required: host_machine.system() == 'darwin')
debug = get_option('debug')

add_project_arguments(
	cxx.get_supported_arguments(
		'-Wvla', '-Wimplicit-fallthrough', '-Wno-attributes'
	), language: 'cpp'
)

if target_machine.system() == 'windows'
	add_project_arguments(
		'-D_WINDOWS',
		language: 'cpp'
	)
endif

if cxx.get_id() == 'msvc'
	add_global_arguments(
		'-D_CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES=1',
		'-D_CRT_NONSTDC_NO_WARNINGS=1',
		'-D_CRT_SECURE_NO_WARNINGS',
		'-wd5030',
		language: ['c', 'cpp']
	)
elif (cxx.get_id() == 'gcc' or cxx.get_id() == 'clang') and not debug
	add_project_arguments(
		'-D_FORTIFY_SOURCE=2',
		language: 'cpp'
	)
endif

substrate = dependency(
	'substrate',
	required: true,
	version: '>=0.0.1',
	default_options: [
		'build_tests=false',
		'cpp_std=c++17',
		'default_library=@0@'.format(get_option('default_library')),
	]
)
substrateDir = meson.current_source_dir() / 'deps/substrate'

crunchMake = find_program('crunchMake', required: false)

coverage = get_option('b_coverage')

if target_machine.system() == 'windows'
	# if MSVC, b_coverage will always return false because
	# Meson doesn't officially support coverage
	if cxx.get_id() == 'msvc' and get_option('buildtype') == 'debug'
		coverageRunner = find_program('OpenCppCoverage', required: false)

		if coverageRunner.found()
			message('Enabling coverage under MSVC')
			coverageArgs = [
				'--sources', '@0@\\libAudio\\*'.format(meson.current_source_dir()),
				'--sources', '@0@\\test\\*'.format(meson.current_source_dir()),
				'--sources', '@0@\\*'.format(meson.current_build_dir()),
				'--modules', meson.current_build_dir(),
				'--export_type'
			]
			coverage = true
		endif
	endif
endif

subdir('libAudio')
if not meson.is_subproject()
	subdir('player')
	subdir('transcoder')
endif
subdir('test', if_found: crunchMake)
if not meson.is_subproject()
	subdir('harness')
	if get_option('bindings')
		subdir('bindings')
	endif
	if get_option('spectrometer')
		subdir('spectrometer')
	endif
endif

runClangTidy = find_program('runClangTidy.py')
run_target(
        'clang-tidy',
        command: [runClangTidy, '-s', meson.current_source_dir(), '-p', meson.current_build_dir()]
)
