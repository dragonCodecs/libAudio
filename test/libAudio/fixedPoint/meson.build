fixedPointTests = [
	'testFineLinearSlideUp',
	'testFineLinearSlideDown',
	'testLinearSlideUp',
	'testLinearSlideDown'
]

fixedPointObj = libAudioLibrary.extract_objects('fixedPoint/fixedPoint.cpp')

foreach test : fixedPointTests
	custom_target(
		test,
		command: [
			crunchMake, '-s', '-z', 'defs', '@INPUT@', '-o', '@OUTPUT@', '-I' + libAudioInclude
		] + commandExtra,
		input: [test + '.cxx', fixedPointObj],
		output: test + '.so',
		build_by_default: true
	)
endforeach

test(
	'testFixedPoint',
	crunchpp,
	args: fixedPointTests,
	workdir: meson.current_build_dir()
)
