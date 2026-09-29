/*
 * The Incredible Machine - reconstruction
 *
 * **Ours, and read only by Turbo C++ 3.0.** That compiler predates C99 and has
 * no <stdint.h>; the port is written in the stdint names because every value
 * in the original has a width it depended on, and this file gives those names
 * their widths *as the original compiler has them*: `int` is sixteen bits and
 * `long` thirty-two. `tools/judge.py` puts this directory first on TCC's
 * include path, so the host never sees it.
 */
#ifndef TC_STDINT_H
#define TC_STDINT_H

typedef signed char    int8_t;
typedef unsigned char  uint8_t;
typedef int            int16_t;
typedef unsigned int   uint16_t;
typedef long           int32_t;
typedef unsigned long  uint32_t;
/* A data pointer is near in the medium model: sixteen bits. */
typedef unsigned int   uintptr_t;

#endif
