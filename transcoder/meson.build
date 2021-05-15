transcoderSrcs = ['transcoder.cxx']

executable(
	'libAudioTranscode',
	transcoderSrcs,
	dependencies: [libAudio, substrate],
	gnu_symbol_visibility: 'inlineshidden',
	install: true
)
