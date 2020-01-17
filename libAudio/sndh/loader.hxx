#ifndef SNDH_LOADER__HXX
#define SNDH_LOADER__HXX

#include <cstdint>
#include "../fd.hxx"

struct sndhEntryPoints_t final
{
	uint32_t init;
	uint32_t exit;
	uint32_t play;
};

struct sndhLoader_t
{
private:
	sndhEntryPoints_t _entryPoints;

public:
	sndhLoader_t(const fd_t &file);
	const sndhEntryPoints_t &entryPoints() const noexcept { return _entryPoints; }
};

#endif /*SNDH_LOADER__HXX*/
