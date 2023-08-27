libAudioTests = [
	'testFixedVector', 'testFD', 'testString', 'testFileInfo'
]

testHelpers = static_library(
	'testHelpers',
	['fixedVector.cxx', 'fd.cxx', 'string.cxx'],
	pic: true,
	dependencies: [libAudio, libcrunchpp, substrate],
	install: false,
	build_by_default: true
)

testObjectMap = {
	'testFixedVector': {'test': ['fixedVector.cxx']},
	'testFD': {'test': ['fd.cxx']},
	'testString': {'test': ['string.cxx']},
	'testFileInfo': {'libAudio': ['fileInfo.cxx']},
}

testIncludes = []
foreach include : libAudioIncludes
	testIncludes += '-I@0@'.format(include)
endforeach

foreach test : libAudioTests
	map = testObjectMap.get(test, {})
	libAudioObjs = map.has_key('libAudio') ? [libAudioLibrary.extract_objects(map['libAudio'])] : []
	testObjs = map.has_key('test') ? [testHelpers.extract_objects(map['test'])] : []
	testLibs = map.get('libs', [])
	custom_target(
		test,
		command: [
			crunchMake, '-s', '@INPUT@', '-o', '@OUTPUT@'
		] + testIncludes + commandExtra + testLibs,
		input: [test + '.cxx'] + libAudioObjs + testObjs,
		output: test + '.so',
		build_by_default: true
	)

	if cxx.get_id() == 'msvc' and coverage
		test(
			test,
			coverageRunner,
			args: coverageArgs + ['cobertura:crunch-none-coverage.xml', '--', crunchpp, test],
			workdir: meson.current_build_dir()
		)
	else
		test(
			test,
			crunchpp,
			args: [test],
			workdir: meson.current_build_dir()
		)
	endif
endforeach

subdir('fixedPoint')
