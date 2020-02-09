crunchMake = find_program('crunchMake', required: false)

if crunchMake.found()
	coverage = get_option('b_coverage')
	crunchpp = find_program('crunch++')
	libcrunchpp = dependency('crunch++')

	if coverage
		crunchCoverage = ['--coverage']
	else
		crunchCoverage = []
	endif

	sanitizer = get_option('b_sanitize')
	if sanitizer != 'none'
		crunchCoverage += ['-fsanitize=' + sanitizer]
	endif

	subdir('libAudio')
endif
