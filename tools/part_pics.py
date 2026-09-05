#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0
"""Turn `TIM_PARTPICS` output into a page of part icons, for naming them.

The pixels come from the game: `devtim` draws each icon with the port's own
transcribed decoders and hands back raw indexed bytes. This only arranges
them - PNG and HTML are presentation, and the rule that the original does the
decoding is what keeps four bitmap formats from being reimplemented here.

    TIM_PARTPICS=out/parts devtim && uv run python tools/part_pics.py

The page is written beside the pixels and is self-contained: every image is a
data URI, so it can be moved or opened from anywhere.
"""

import argparse
import base64
import os
import sys

import png as pngmod

W, H = 64, 48


def main():
    ap = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""environment:
  none. The directory is the only input, and TIM_PARTPICS wrote it.
""")
    ap.add_argument("--dir", default="out/parts",
                    help="where TIM_PARTPICS wrote its files (default out/parts)")
    ap.add_argument("--scale", type=int, default=3,
                    help="pixel scale for the page (default 3, so 64x48 shows "
                         "at 192x144 - these are small icons)")
    args = ap.parse_args()

    pal_path = os.path.join(args.dir, "palette.bin")
    if not os.path.exists(pal_path):
        sys.exit(f"{pal_path} is not there - run devtim with TIM_PARTPICS first")
    # png.indexed_rows wants a sequence of (r, g, b); the dump is flat bytes.
    flat = open(pal_path, "rb").read()
    palette = [tuple(flat[i * 3:i * 3 + 3]) for i in range(256)]

    if not any(any(c) for c in palette):
        sys.exit("the palette is all black - devtim wrote it before the game "
                 "made tim.pal active, so every icon would render blank")

    raws = sorted(f for f in os.listdir(args.dir) if f.endswith(".raw"))
    if not raws:
        sys.exit(f"no .raw icons in {args.dir}")

    cards = []
    for name in raws:
        kind = int(name[5:7])
        px = open(os.path.join(args.dir, name), "rb").read()
        if len(px) < W * H:
            continue

        # Blank icons are worth showing as blank rather than skipping: a gap in
        # the numbering would make the kind numbers wrong for everything after.
        blank = len(set(px)) <= 1

        # `save_indexed` writes the file and answers its size; the PNGs are
        # worth keeping on disk as well as embedding, so this uses it rather
        # than assembling rows by hand.
        png_path = os.path.join(args.dir, name[:-4] + ".png")
        pngmod.save_indexed(png_path, px, W, H, palette, scale=args.scale)
        uri = base64.b64encode(open(png_path, "rb").read()).decode()

        cards.append((kind, uri, blank))

    out = os.path.join(args.dir, "index.html")
    with open(out, "w") as f:
        f.write("""<!doctype html><meta charset="utf-8">
<title>The Incredible Machine - part icons</title>
<style>
 body { background:#1b1b1b; color:#ddd; font:14px system-ui, sans-serif;
        margin:0; padding:24px; }
 h1   { font-size:18px; font-weight:600; margin:0 0 4px; }
 p    { color:#999; margin:0 0 24px; max-width:60em; }
 .g   { display:grid; gap:16px;
        grid-template-columns:repeat(auto-fill, minmax(210px, 1fr)); }
 .c   { background:#262626; border:1px solid #333; border-radius:6px;
        padding:10px; }
 .c img { display:block; width:100%; height:auto; image-rendering:pixelated;
          background:#000; border-radius:3px; }
 .k   { color:#7ab; font-weight:600; margin-top:8px; }
 .b   { opacity:.35; }
 .n   { color:#777; font-size:12px; }
</style>
<h1>Part icons</h1>
<p>Drawn by the port itself from <code>icons.bmp</code>, the list at DGROUP
0x4ec7 that the parts bin walks, so the number under each picture is the
game's own kind number. Faded cards drew nothing.</p>
<div class=g>
""")
        for kind, uri, blank in cards:
            cls = "c b" if blank else "c"
            note = '<div class=n>drew nothing</div>' if blank else ''
            f.write(f'<div class="{cls}">'
                    f'<img src="data:image/png;base64,{uri}" alt="kind {kind}">'
                    f'<div class=k>kind {kind}</div>{note}</div>\n')
        f.write("</div>\n")

    n_blank = sum(1 for _, _, b in cards if b)
    print(f"wrote {out}")
    print(f"  {len(cards)} icons, {n_blank} of them blank")


if __name__ == "__main__":
    sys.exit(main())
