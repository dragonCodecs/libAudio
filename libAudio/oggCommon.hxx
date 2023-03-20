// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2019-2023 Rachel Mant <git@dragonmux.network>
#ifndef OGG_COMMON_HXX
#define OGG_COMMON_HXX

#include <ogg/ogg.h>

bool isOgg(const int32_t fd, ogg_packet &headerPacket) noexcept;
bool isVorbis(ogg_packet &headerPacket) noexcept;
bool isFLAC(ogg_packet &headerPacket) noexcept;
bool isOpus(ogg_packet &headerPacket) noexcept;

#endif /*OGG_COMMON_HXX*/
