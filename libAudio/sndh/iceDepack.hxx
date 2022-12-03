// SPDX-License-Identifier: BSD-3-Clause
#ifndef SNDH_ICE_DEPACK_HXX
#define SNDH_ICE_DEPACK_HXX

#include <memory>
#include <substrate/fd>

using substrate::fd_t;

struct sndhDepacker_t final
{
private:
	std::unique_ptr<char []> _data{};

	bool depack(const fd_t &file) noexcept;

public:
	sndhDepacker_t(const fd_t &file);
	[[nodiscard]] bool valid() const noexcept { return static_cast<bool>(_data); }
};

#endif /*SNDH_ICE_DEPACK_HXX*/
