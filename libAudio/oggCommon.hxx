#ifndef OGG_COMMON__HXX
#define OGG_COMMON__HXX

#include <ogg/ogg.h>

bool isOgg(const int32_t fd, ogg_packet &headerPacket) noexcept;
bool isVorbis(const ogg_packet &headerPacket) noexcept;
bool isOpus(const ogg_packet &headerPacket) noexcept;

#endif /*OGG_COMMON__HXX*/
