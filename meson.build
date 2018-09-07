project(
	'libAudio',
	'cpp',
	default_options: [
		'cpp_std=c++11',
		'warning_level=3',
		'buildtype=release'
	],
	version: '0.2.1'
)

if meson.version() < '0.42.0'
	error('Meson is too old for this library')
endif

compiler = meson.get_compiler('cpp')

subdir('libAudio')
