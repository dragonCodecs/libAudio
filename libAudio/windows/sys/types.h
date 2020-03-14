#ifndef SYS_TYPES__H
#define SYS_TYPES__H

#include <cstdint>

#define _INO_T_DEFINED
using ino_t = uint16_t;
using _ino_t = ino_t;

#define _DEV_T_DEFINED
using dev_t = uint32_t;
using _dev_t = dev_t;

#define _OFF_T_DEFINED
using off_t = uint64_t;
using _off_t = off_t;

#endif /*SYS_TYPES__H*/
