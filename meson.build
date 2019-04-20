project(
	'libAudio',
	'cpp',
	default_options: [
		'cpp_std=c++11',
		'warning_level=3',
		'buildtype=release'
	],
	version: '0.4.0'
)

if meson.version() < '0.47.0'
	error('Meson is too old for this library')
endif

cxx = meson.get_compiler('cpp')

add_project_arguments('-fvisibility=hidden', '-fvisibility-inlines-hidden', language: 'cpp')
add_project_arguments(
	cxx.get_supported_arguments(
		'-Wvla', '-Wimplicit-fallthrough', '-Wno-attributes'
	), language: 'cpp'
)

if cxx.get_id() == 'msvc'
	add_project_arguments(
		'-D_CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES=1',
		'-D_CRT_NONSTDC_NO_WARNINGS=1',
		language: 'cpp')
endif

subdir('libAudio')
subdir('player')
subdir('transcoder')
