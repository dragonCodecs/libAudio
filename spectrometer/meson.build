openGL = dependency('GL')

qt5 = import('qt5')
qt5Libs = dependency('qt5', modules: ['Core', 'Gui', 'Widgets'])

if target_machine.system() == 'windows'
	qt5Libs += dependency('qt5', modules: ['Main'])
endif

spectrometerSrcs = [
	'spectrometer.cxx'
]

spectrometerMoc = [
	'spectrometer.hxx'
]

spectrometerUIs = [
	'spectrometer.ui'
]

spectrometerSrcs += qt5.preprocess(
	ui_files: spectrometerUIs,
	moc_headers: spectrometerMoc
)

spectrometer = executable(
	'spectrometer',
	spectrometerSrcs,
	dependencies: [libAudio, openGL, qt5Libs, threading],
	gnu_symbol_visibility: 'inlineshidden'
)
