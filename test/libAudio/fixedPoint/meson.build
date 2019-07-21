fixedPointTests = [
	'testFineLinearSlideUp',
	'testFineLinearSlideDown'
]

fixedPointObj = libAudioLibrary.extract_objects('fixedPoint/fixedPoint.cpp')

foreach test : fixedPointTests
	custom_target(
		test,
		command: [crunchMake, '-s', '@INPUT@', '-o', '@OUTPUT@', '-I' + libAudioInclude],
		input: [test + '.cxx', fixedPointObj],
		output: test + '.so',
		build_by_default: true
	)
endforeach

test(
	'fixedPoint',
	crunchpp,
	args: fixedPointTests,
	workdir: meson.current_build_dir()
)