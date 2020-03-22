crunchpp = find_program('crunch++')
libcrunchpp = dependency('crunch++')

commandExtra = ['-I' + substrateDir]
if get_option('b_coverage')
	commandExtra += ['--coverage']
endif
if get_option('debug')
	commandExtra += ['--debug']
endif
if cxx.get_id() != 'msvc'
	commandExtra += ['-z', 'defs']
endif

sanitizer = get_option('b_sanitize')
if sanitizer != 'none'
	commandExtra += ['-fsanitize=' + sanitizer]
endif

subdir('libAudio')
