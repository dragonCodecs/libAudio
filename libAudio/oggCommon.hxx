#ifndef OGG_COMMON__HXX
#define OGG_COMMON__HXX

#include <ogg/ogg.h>

bool isOgg(const int32_t fd, ogg_packet &headerPacket) noexcept;
bool isVorbis(ogg_packet &headerPacket) noexcept;
bool isOpus(ogg_packet &headerPacket) noexcept;

#endif /*OGG_COMMON__HXX*/
