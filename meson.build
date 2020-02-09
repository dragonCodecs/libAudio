project(
	'libAudio',
	'cpp',
	default_options: [
		'cpp_std=c++11',
		'warning_level=3',
		'buildtype=release',
		'strip=true',
		'b_ndebug=if-release',
		'b_lto=true'
	],
	version: '0.5.2',
	meson_version: '>= 0.52'
)

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
subdir('test')
subdir('harness')
if get_option('spectrometer')
	subdir('spectrometer')
endif
