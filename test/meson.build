crunchpp = find_program('crunch++')
libcrunchpp = dependency('crunch++')

commandExtra = [
	'-std=c++17',
	'-lcrunch++',
	'-I@0@'.format(substrateDir),
]
if coverage
	commandExtra += ['--coverage']
endif
if get_option('debug')
	commandExtra += ['--debug']
endif
if get_option('b_lundef') and cxx.has_link_argument('-z defs')
	commandExtra += ['-z', 'defs']
endif
if get_option('b_lto')
	commandExtra += ['-flto']
endif

sanitizer = get_option('b_sanitize')
if sanitizer != 'none'
	commandExtra += ['-fsanitize=' + sanitizer]
endif

subdir('libAudio')
