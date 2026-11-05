#!/usr/bin/env python3
import struct, sys
if len(sys.argv) != 4:
    sys.exit("usage: packlcs.py <input.bin> <name> <output.lcs>")
inp, name, out = sys.argv[1], sys.argv[2].encode()[:63], sys.argv[3]
with open(inp, 'rb') as f: payload = f.read()
hdr  = b"LCS1"
hdr += struct.pack("<I", 1)             # version
hdr += struct.pack("<I", 0)             # flags
hdr += struct.pack("<I", len(name))     # name_len
hdr += struct.pack("<I", 0)             # entry_offset
hdr += struct.pack("<I", len(payload))  # payload_size
hdr += name.ljust(64, b'\x00')
with open(out, 'wb') as f: f.write(hdr + payload)