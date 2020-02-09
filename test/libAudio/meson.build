libAudioTests = [
	'testFixedVector'
]

testHelpers = static_library(
	'testHelpers',
	['fixedVector.cxx'],
	pic: true,
	dependencies: [libAudio, libcrunchpp],
	install: false,
	build_by_default: true
)

testObjectMap = {
	'testFixedVector': {'test': ['fixedVector.cxx']}
}

foreach test : libAudioTests
	map = testObjectMap.get(test, {})
	libAudioObjs = map.has_key('libAudio') ? [libAudioLibrary.extract_objects(map['libAudio'])] : []
	testObjs = map.has_key('test') ? [testHelpers.extract_objects(map['test'])] : []
	testLibs = map.get('libs', [])
	custom_target(
		test,
		command: [
			crunchMake, '-s', '-z', 'defs', '@INPUT@', '-o', '@OUTPUT@', '-I' + libAudioInclude
		] + crunchCoverage + testLibs,
		input: [test + '.cxx'] + libAudioObjs + testObjs,
		output: test + '.so',
		build_by_default: true
	)

	test(
		test,
		crunchpp,
		args: [test],
		workdir: meson.current_build_dir()
	)
endforeach

subdir('fixedPoint')
