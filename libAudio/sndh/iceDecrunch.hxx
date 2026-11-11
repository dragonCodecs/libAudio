// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2022-2023 Rachel Mant <git@dragonmux.network>
#ifndef SNDH_ICE_DEPACK_HXX
#define SNDH_ICE_DEPACK_HXX

#include <substrate/fd>
#include <substrate/fixed_vector>
#include <substrate/span>

using substrate::fd_t;
using substrate::fixedVector_t;

struct SndhDecruncher final
{
	size_t length() const;
	bool seek_start();
	bool read(substrate::span<uint8_t> span);
};

#endif /*SNDH_ICE_DEPACK_HXX*/
