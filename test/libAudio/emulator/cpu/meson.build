cpuTests = []

if formats['SNDH']
cpuTests += [
	'testM68k'
]
endif

testObjectMap = {
	'testM68k': {'libAudio': ['emulator/cpu/m68k.cxx', 'console.cxx'], 'deps': ['testM68kDecodeTable.hxx']},
}

foreach test : cpuTests
	map = testObjectMap.get(test, {})
	libAudioObjs = map.has_key('libAudio') ? [libAudioLibrary.extract_objects(map['libAudio'])] : []
	#testObjs = map.has_key('test') ? [testHelpers.extract_objects(map['test'])] : []
	testLibs = map.get('libs', [])
	testDeps = map.get('deps', [])
	custom_target(
		test,
		command: [
			crunchMake, '-s', '@INPUT@', '-o', '@OUTPUT@'
		] + testIncludes + commandExtra + testLibs,
		input: [test + '.cxx'] + libAudioObjs,# + testObjs,
		output: test + '.so',
		depend_files: testDeps,
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
