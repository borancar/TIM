/*
 * SHA-256, so a frame can be compared without being kept.
 *
 * NOT a transcription - the original has no such thing. It is here because
 * `tools/check_native.py` asks one question of every frame, "is this byte for
 * byte that one", and answering it by writing 308 KB per frame and reading it
 * back costs 222 MB a run and has filled this machine's disk twice. A digest
 * answers the same question in 32 bytes.
 *
 * FIPS 180-4. Checked against Python's hashlib on real frames, because a hash
 * that is subtly wrong agrees with itself perfectly and would make every
 * comparison here vacuous.
 */
#ifndef SHA256_H
#define SHA256_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint32_t h[8];
    uint64_t len;              /* bytes fed so far */
    uint8_t  buf[64];
    size_t   used;
} sha256_t;

void sha256_init(sha256_t *s);
void sha256_update(sha256_t *s, const void *data, size_t n);
/* 32 bytes out. */
void sha256_final(sha256_t *s, uint8_t out[32]);

#endif
