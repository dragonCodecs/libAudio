// SPDX-License-Identifier: BSD-3-Clause
#include "mp3.hxx"

/*!
 * @internal
 * @file saveMP3.cxx
 * @brief The implementation of the MP3 encoder API
 * @author Rachel Mant <git@dragonmux.network>
 * @date 2022
 */

mp3_t::mp3_t(fd_t &&fd, audioModeWrite_t) noexcept : audioFile_t{audioType_t::mp3, std::move(fd)},
	encoderCtx{makeUnique<encoderContext_t>()} { }
mp3_t::encoderContext_t::encoderContext_t() noexcept : encoder{lame_init()} { }

mp3_t::encoderContext_t::~encoderContext_t() noexcept
{
	lame_close(encoder);
}
