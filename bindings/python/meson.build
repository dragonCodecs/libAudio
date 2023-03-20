# SPDX-License-Identifier: BSD-3-Clause
# SPDX-FileCopyrightText: 2020-2023 Rachel Mant <git@dragonmux.network>
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
	dependencies: [libAudio, pythonDep, substrate],
	gnu_symbol_visibility: 'inlineshidden',
	install: true,
	install_dir: python.get_install_dir()
)
