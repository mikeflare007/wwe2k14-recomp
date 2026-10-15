#!/usr/bin/env python3
"""Extract an Xbox 360 title update package (STFS "LIVE"/"PIRS"/"CON", e.g. tu00000002_00000000)
into a folder, keeping its sub-folders. WWE 2K14 Recomp reads the title update's files from the
`update/` folder next to the game (REX_UPDATE_DATA_ROOT), so:

    python3 tools/extract-title-update.py tu00000002_00000000 path/to/update

Only reads your own file; nothing is downloaded. Supports the read-only, consecutive-block layout
title updates use (it stops with an error on anything else rather than writing a broken file).
"""
import os
import struct
import sys


def main():
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    src, outdir = sys.argv[1], sys.argv[2]
    d = open(src, "rb").read()
    if d[:4] not in (b"LIVE", b"PIRS", b"CON "):
        sys.exit(f"{src}: not an STFS package (magic {d[:4]!r})")
    header_size = struct.unpack(">I", d[0x340:0x344])[0]
    media_id, version, _, title_id = struct.unpack(">IIII", d[0x354:0x364])
    print(f"title {title_id:08X}  media {media_id:08X}  version {version:#010x}")
    vd = d[0x379:0x379 + 0x24]
    ft_count = struct.unpack("<H", vd[3:5])[0]
    ft_block = vd[5] | vd[6] << 8 | vd[7] << 16
    base_off = (header_size + 0xFFF) & ~0xFFF

    def block_offset(idx, shift):
        blk, base = idx, 0xAA
        for _ in range(3):
            blk += ((idx + base) // base) << shift
            if idx < base:
                break
            base *= 0xAA
        return base_off + (blk << 12)

    entries = []
    for shift in (0, 1):
        entries = []
        for i in range(ft_count):
            off = block_offset(ft_block + i, shift)
            for j in range(64):
                e = d[off + j * 0x40: off + (j + 1) * 0x40]
                if len(e) < 0x40 or e[0] == 0:
                    continue
                flags = e[0x28]
                name = e[: flags & 0x3F].decode("latin1")
                start = e[0x2F] | e[0x30] << 8 | e[0x31] << 16
                parent = struct.unpack(">h", e[0x32:0x34])[0]
                size = struct.unpack(">I", e[0x34:0x38])[0]
                entries.append((name, flags, start, size, parent))
        if entries and all(n.isprintable() for n, *_ in entries):
            break
    else:
        sys.exit("could not read the file table")

    def path_of(index):
        parts = []
        while index != -1:
            name, _, _, _, parent = entries[index]
            parts.append(name)
            index = parent
        return os.path.join(*reversed(parts))

    for i, (name, flags, start, size, parent) in enumerate(entries):
        rel = path_of(i)
        dest = os.path.join(outdir, rel)
        if flags & 0x80:
            os.makedirs(dest, exist_ok=True)
            continue
        if not flags & 0x40 and size > 0x1000:
            sys.exit(f"{rel}: non-consecutive blocks are not supported")
        nblocks = (size + 0xFFF) // 0x1000
        data = b"".join(d[block_offset(start + k, shift): block_offset(start + k, shift) + 0x1000]
                        for k in range(nblocks))[:size]
        os.makedirs(os.path.dirname(dest) or ".", exist_ok=True)
        with open(dest, "wb") as f:
            f.write(data)
        print(f"  {rel}  ({size} bytes)")


if __name__ == "__main__":
    main()
