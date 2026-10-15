#!/usr/bin/env python3
"""Install your own WWE 2K14 DLC packages (Xbox 360 STFS "LIVE"/"PIRS"/"CON" files) for WWE 2K14 Recomp.

    python tools/install-dlc.py <package file or folder> [more ...] [--userdata PATH]

Each package is unpacked to

    <userdata>/0000000000000000/545408B2/00000002/<package file name>/          (the DLC's files)
    <userdata>/0000000000000000/545408B2/Headers/00000002/<package file name>.header

which is where the game's content enumeration looks for marketplace content (the same layout Xenia uses).
`--userdata` defaults to the `userdata` folder next to play-wwe2k14.bat (the parent of this tools/ folder,
or the current folder when run from the game folder).

Only reads files you supply; nothing is downloaded. Packages for other games are skipped. Re-running replaces an
installed package. To remove a DLC, delete its folder and its .header file.
"""
import argparse
import os
import shutil
import struct
import sys

WWE2K14_TITLE_ID = 0x545408B2
MARKETPLACE_CONTENT = 0x00000002
BLOCK = 0x1000


class Stfs:
    def __init__(self, path):
        self.path = path
        with open(path, "rb") as f:
            self.d = f.read()
        d = self.d
        if d[:4] not in (b"LIVE", b"PIRS", b"CON "):
            raise ValueError(f"not an STFS package (magic {d[:4]!r})")
        self.header_size = struct.unpack(">I", d[0x340:0x344])[0]
        self.content_type = struct.unpack(">I", d[0x344:0x348])[0]
        self.title_id = struct.unpack(">I", d[0x360:0x364])[0]
        vd = d[0x379:0x379 + 0x24]
        self.read_only = bool(vd[2] & 1)
        self.ft_count = struct.unpack("<H", vd[3:5])[0]
        self.ft_block = vd[5] | vd[6] << 8 | vd[7] << 16
        self.display_name = d[0x411:0x411 + 0x80].decode("utf-16-be", "ignore").split("\0")[0]
        self.base = (self.header_size + 0xFFF) & ~0xFFF
        # Read-only packages store one hash table per level; writable ones store two (shift 1).
        self.shift = 0 if self.read_only else 1
        self.entries = self._read_file_table()

    def _block_offset(self, idx):
        blk, base = idx, 0xAA
        for _ in range(3):
            blk += ((idx + base) // base) << self.shift
            if idx < base:
                break
            base *= 0xAA
        return self.base + (blk << 12)

    def _hash_entry_offset(self, idx):
        # Level-0 hash table that covers block idx sits right before its 0xAA data blocks.
        group = idx // 0xAA
        tbl = group * 0xAB
        if group:
            tbl += ((group + 0xAA) // 0xAA) << self.shift
            if group >= 0xAA:
                tbl += ((group + 0x70E4) // 0x70E4) << self.shift
        tbl = self.base + (tbl << 12)
        return tbl + (idx % 0xAA) * 0x18

    def _next_block(self, idx):
        e = self._hash_entry_offset(idx)
        return self.d[e + 0x15] << 16 | self.d[e + 0x16] << 8 | self.d[e + 0x17]

    def _read_chain(self, start, count):
        out, blk = [], start
        for _ in range(count):
            out.append(blk)
            blk = self._next_block(blk)
        return out

    def _read_file_table(self):
        blocks = self._read_chain(self.ft_block, self.ft_count)
        entries = []
        for b in blocks:
            off = self._block_offset(b)
            for j in range(64):
                e = self.d[off + j * 0x40: off + (j + 1) * 0x40]
                if len(e) < 0x40 or e[0] == 0:
                    continue
                flags = e[0x28]
                entries.append({
                    "name": e[: flags & 0x3F].decode("latin1"),
                    "dir": bool(flags & 0x80),
                    "consecutive": bool(flags & 0x40),
                    "blocks": e[0x29] | e[0x2A] << 8 | e[0x2B] << 16,
                    "start": e[0x2F] | e[0x30] << 8 | e[0x31] << 16,
                    "parent": struct.unpack(">h", e[0x32:0x34])[0],
                    "size": struct.unpack(">I", e[0x34:0x38])[0],
                })
        if not entries or not all(x["name"].isprintable() for x in entries):
            raise ValueError("could not read the package's file table")
        return entries

    def path_of(self, i):
        parts, seen = [], set()
        while i >= 0 and i not in seen:
            seen.add(i)
            parts.append(self.entries[i]["name"])
            i = self.entries[i]["parent"]
        return os.path.join(*reversed(parts))

    def read(self, entry):
        n = (entry["size"] + BLOCK - 1) // BLOCK
        if entry["consecutive"]:
            blocks = [entry["start"] + k for k in range(n)]
        else:
            blocks = self._read_chain(entry["start"], n)
        data = b"".join(self.d[self._block_offset(b): self._block_offset(b) + BLOCK] for b in blocks)
        return data[: entry["size"]]


def content_header(display_name, file_name):
    """XCONTENT_AGGREGATE_DATA (328 bytes, big-endian like the guest) + 4-byte license mask."""
    h = bytearray(328)
    struct.pack_into(">II", h, 0, 1, MARKETPLACE_CONTENT)       # device_id (HDD), content_type
    name16 = display_name.encode("utf-16-be")[:254]
    h[8:8 + len(name16)] = name16                                # display name (128 UTF-16 chars)
    fn = file_name.encode("ascii", "ignore")[:42]
    h[264:264 + len(fn)] = fn                                    # file name (42 chars)
    struct.pack_into(">Q", h, 312, 0)                            # xuid 0 = shared content
    struct.pack_into(">I", h, 320, WWE2K14_TITLE_ID)
    return bytes(h) + b"\xff\xff\xff\xff"                        # license mask: all granted


def install(pkg_path, userdata):
    pkg = Stfs(pkg_path)
    name = os.path.basename(pkg_path)
    if pkg.title_id != WWE2K14_TITLE_ID:
        print(f"  skip {name}: title {pkg.title_id:08X} is not WWE 2K14 (545408B2)")
        return False
    if pkg.content_type != MARKETPLACE_CONTENT:
        kind = "title update (use tools/extract-title-update.py)" if pkg.content_type == 0xB0000 else f"content type {pkg.content_type:#x}"
        print(f"  skip {name}: {kind}, not DLC")
        return False
    root = os.path.join(userdata, "0000000000000000", f"{WWE2K14_TITLE_ID:08X}")
    dest = os.path.join(root, f"{MARKETPLACE_CONTENT:08X}", name)
    if os.path.isdir(dest):
        shutil.rmtree(dest)
    os.makedirs(dest)
    count = 0
    for i, e in enumerate(pkg.entries):
        out = os.path.join(dest, pkg.path_of(i))
        if e["dir"]:
            os.makedirs(out, exist_ok=True)
            continue
        os.makedirs(os.path.dirname(out), exist_ok=True)
        with open(out, "wb") as f:
            f.write(pkg.read(e))
        count += 1
    hdr_dir = os.path.join(root, "Headers", f"{MARKETPLACE_CONTENT:08X}")
    os.makedirs(hdr_dir, exist_ok=True)
    with open(os.path.join(hdr_dir, name + ".header"), "wb") as f:
        f.write(content_header(pkg.display_name or name, name))
    print(f"  installed {name}: \"{pkg.display_name}\" ({count} files)")
    return True


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("packages", nargs="+", help="DLC package files, or folders containing them")
    here = os.path.dirname(os.path.abspath(__file__))
    default_ud = os.path.join(os.path.dirname(here), "userdata") if os.path.basename(here) == "tools" \
        else os.path.join(here, "userdata")
    ap.add_argument("--userdata", default=default_ud, help=f"userdata folder (default: {default_ud})")
    args = ap.parse_args()

    files = []
    for p in args.packages:
        if os.path.isdir(p):
            for dirpath, _, names in os.walk(p):
                files += [os.path.join(dirpath, n) for n in sorted(names)]
        else:
            files.append(p)
    ok = 0
    print(f"Installing into {args.userdata}")
    for f in files:
        try:
            ok += install(f, args.userdata)
        except (ValueError, OSError, IndexError, struct.error) as ex:
            print(f"  skip {os.path.basename(f)}: {ex}")
    print(f"Done: {ok} DLC package(s) installed.")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
