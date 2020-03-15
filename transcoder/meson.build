transcoderSrcs = ['transcoder.cxx']

executable(
	'libAudioTranscode',
	transcoderSrcs,
	dependencies: libAudio,
	include_directories: includes,
	gnu_symbol_visibility: 'inlineshidden',
	install: true
)
