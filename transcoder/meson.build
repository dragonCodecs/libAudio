# SPDX-License-Identifier: BSD-3-Clause
# SPDX-FileCopyrightText: 2019-2023 Rachel Mant <git@dragonmux.network>
transcoderSrcs = ['transcoder.cxx']

executable(
	'libAudioTranscode',
	transcoderSrcs,
	dependencies: [libAudio, substrate],
	gnu_symbol_visibility: 'inlineshidden',
	install: true
)
