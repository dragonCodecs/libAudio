playerSrcs = ['player.cxx']
playerDefs = []
if get_option('streaming')
	playerDefs += ['-DSTREAMING']
endif

executable(
	'libAudio',
	playerSrcs,
	cpp_args: playerDefs,
	dependencies: [libAudio, substrate],
	gnu_symbol_visibility: 'inlineshidden',
	install: true
)
