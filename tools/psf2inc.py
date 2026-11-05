#!/usr/bin/env python3
import struct, sys

with open(sys.argv[1], 'rb') as f: d = f.read()

if d[:2] == b'\x36\x04':
    charsize = d[3]
    numglyph = 512 if (d[2] & 1) else 256
    glyphs   = d[4:4 + numglyph * charsize]
    h, w = charsize, 8
elif d[:4] == b'\x72\xB5\x4A\x86':
    _, hsz, _, n, bpg, h, w = struct.unpack('<IIIIIII', d[4:32])
    glyphs = d[hsz:hsz + n * bpg]
    charsize = bpg
    numglyph = n
else:
    sys.exit("unknown PSF format")

count = min(128, numglyph)
print(f"#define FONT_W {w}")
print(f"#define FONT_H {h}")
print(f"#define FONT_COUNT {count}")
print("static const unsigned char font_data[] = {")
for i in range(count):
    row = ",".join(f"0x{b:02X}" for b in glyphs[i*charsize:i*charsize+charsize])
    print(f" {row},")
print("};")
