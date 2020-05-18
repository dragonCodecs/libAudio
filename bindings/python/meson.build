pythonDep = python.dependency()
if not pythonDep.found()
	subdir_done()
endif

libAudioPythonSrcs = [
	'libAudioPython.cxx', 'audioFile.cxx'
]

libAudioPython = python.extension_module(
	'libAudio',
	libAudioPythonSrcs,
	dependencies: [libAudio, pythonDep],
	include_directories: [substrate],
	gnu_symbol_visibility: 'inlineshidden',
	install: true,
	install_dir: python.get_install_dir()
)
