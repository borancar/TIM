"""A PNG writer, and the palette conversion the comparisons need.

Deliberately no Pillow and no numpy. A reconstruction's whole claim is that its
measurements are reproducible, and a picture-drawing dependency is a poor thing
to stake that on; the encoder is twenty lines of zlib and struct.

Scaling is nearest-neighbour only. The point of these images is to see
individual pixels, and an interpolated diff image is a lie about which ones
differ.

This file is the port's own tooling; it is not a transcription.
"""
import struct
import zlib


def write_png(path, w, h, rows):
    """rows: h bytearrays of 3*w bytes, RGB."""
    raw = bytearray()
    for r in rows:
        raw.append(0)                     # filter type 0
        raw += r

    def chunk(tag, data):
        return (struct.pack(">I", len(data)) + tag + data
                + struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF))

    with open(path, "wb") as f:
        f.write(b"\x89PNG\r\n\x1a\n")
        f.write(chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 2, 0, 0, 0)))
        f.write(chunk(b"IDAT", zlib.compress(bytes(raw), 9)))
        f.write(chunk(b"IEND", b""))


def indexed_rows(indices, w, h, palette, x0=0, x1=None, y0=0, y1=None, scale=1):
    """Render 8-bit indices through `palette` (a list of (r,g,b)) to RGB rows."""
    x1 = w if x1 is None else x1
    y1 = h if y1 is None else y1
    rows = []
    for y in range(y0, y1):
        row = bytearray()
        base = y * w
        for x in range(x0, x1):
            r, g, b = palette[indices[base + x] & 0xFF]
            row += bytes((r, g, b)) * scale
        for _ in range(scale):
            rows.append(row)
    return rows, (x1 - x0) * scale, (y1 - y0) * scale


def save_indexed(path, indices, w, h, palette, **kw):
    rows, ow, oh = indexed_rows(indices, w, h, palette, **kw)
    write_png(path, ow, oh, rows)
    return ow, oh
