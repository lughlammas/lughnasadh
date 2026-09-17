#!/usr/bin/env python3
"""Stage a Chess-Engine-style APK from a 0.1.0 skeleton + new arm64 .so."""
from __future__ import annotations

import argparse
import struct
import zipfile
from pathlib import Path

ENGINE_NAME = "Lughnasadh 0.2.0 Second Harvest"
VERSION_NAME = "0.2.0"


def encode_utf8_string(s: str) -> bytes:
    data = s.encode("utf-8")
    n = len(data)
    if n >= 0x80:
        raise ValueError("string too long for short UTF-8 pool encoding")
    return bytes([n, n]) + data + b"\x00"


def build_enginelist_axml(name: str = ENGINE_NAME) -> bytes:
    """Rebuild binary enginelist.xml; node section copied from reference template if provided via offsets."""
    strings = [
        name,
        "arm64-v8a",
        "engine",
        "enginelist",
        "filename",
        "liblughnasadh.so",
        "name",
        "target",
    ]
    str_blobs = [encode_utf8_string(s) for s in strings]
    str_data = b"".join(str_blobs)
    while len(str_data) % 4:
        str_data += b"\x00"

    n = len(strings)
    header_size = 0x1C
    offsets_size = 4 * n
    strings_start = header_size + offsets_size
    offsets, off = [], 0
    for blob in str_blobs:
        offsets.append(off)
        off += len(blob)
    pool_size = strings_start + len(str_data)
    flags_utf8 = 1 << 8
    string_pool = struct.pack(
        "<HHIIIIII",
        0x0001,
        header_size,
        pool_size,
        n,
        0,
        flags_utf8,
        strings_start,
        0,
    )
    string_pool += struct.pack("<" + "I" * n, *offsets)
    string_pool += str_data

    # Minimal XML node tree matching Chess Engine enginelist layout
    # (start namespace omitted; start enginelist → start engine + 3 attrs → end engine → end enginelist)
    # Prefer cloning nodes from a reference binary if available.
    return string_pool  # placeholder — caller merges with reference nodes


def rebuild_enginelist_from_reference(ref: bytes, new_name: str) -> bytes:
    xml_type, xml_hs, _ = struct.unpack_from("<HHI", ref, 0)
    sp_off = xml_hs
    _, _, sp_size = struct.unpack_from("<HHI", ref, sp_off)
    nodes = ref[sp_off + sp_size :]

    strings = [
        new_name,
        "arm64-v8a",
        "engine",
        "enginelist",
        "filename",
        "liblughnasadh.so",
        "name",
        "target",
    ]
    str_blobs = [encode_utf8_string(s) for s in strings]
    str_data = b"".join(str_blobs)
    while len(str_data) % 4:
        str_data += b"\x00"
    n = len(strings)
    header_size = 0x1C
    strings_start = header_size + 4 * n
    offsets, off = [], 0
    for blob in str_blobs:
        offsets.append(off)
        off += len(blob)
    pool_size = strings_start + len(str_data)
    string_pool = struct.pack(
        "<HHIIIIII",
        0x0001,
        header_size,
        pool_size,
        n,
        0,
        1 << 8,
        strings_start,
        0,
    )
    string_pool += struct.pack("<" + "I" * n, *offsets)
    string_pool += str_data
    total = xml_hs + len(string_pool) + len(nodes)
    return struct.pack("<HHI", xml_type, xml_hs, total) + string_pool + nodes


def patch_manifest_version_name(manifest: bytes, new_version: str, old_hint: str = "0.1.0 First Harvest") -> bytes:
    data = bytearray(manifest)
    needle = old_hint.encode("utf-16le")
    idx = data.find(needle)
    if idx < 0:
        # already patched?
        if new_version.encode("utf-16le") in data:
            return bytes(data)
        raise SystemExit("versionName string not found in AndroidManifest.xml")
    old_len = struct.unpack_from("<H", data, idx - 2)[0]
    if old_len != len(old_hint):
        raise SystemExit(f"unexpected versionName length {old_len}")
    if len(new_version) > old_len:
        raise SystemExit("new versionName longer than old slot; extend string pool instead")
    struct.pack_into("<H", data, idx - 2, len(new_version))
    payload = new_version.encode("utf-16le") + b"\x00\x00"
    data[idx : idx + len(payload)] = payload
    end = idx + old_len * 2 + 2
    data[idx + len(payload) : end] = b"\x00" * (end - idx - len(payload))
    return bytes(data)


def write_apk(stage: Path, out: Path, ref_apk: Path | None) -> None:
    order = [
        "AndroidManifest.xml",
        "res/mipmap-mdpi-v4/ic_launcher.png",
        "res/mipmap-hdpi-v4/ic_launcher.png",
        "res/mipmap-xhdpi-v4/ic_launcher.png",
        "res/mipmap-xxhdpi-v4/ic_launcher.png",
        "res/mipmap-xxxhdpi-v4/ic_launcher.png",
        "res/xml/enginelist.xml",
        "resources.arsc",
        "classes.dex",
        "lib/arm64-v8a/liblughnasadh.so",
    ]
    comp = {}
    if ref_apk and ref_apk.is_file():
        with zipfile.ZipFile(ref_apk) as z:
            for i in z.infolist():
                if not i.filename.startswith("META-INF"):
                    comp[i.filename] = i.compress_type
    with zipfile.ZipFile(out, "w") as z:
        for name in order:
            data = (stage / name).read_bytes()
            info = zipfile.ZipInfo(name)
            info.compress_type = comp.get(
                name,
                zipfile.ZIP_STORED if name.endswith((".so", ".png", ".arsc")) else zipfile.ZIP_DEFLATED,
            )
            info.date_time = (2026, 9, 17, 12, 0, 0)
            z.writestr(info, data)
    print(f"Wrote {out} ({out.stat().st_size} bytes)")


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--skeleton", type=Path, required=True, help="Extracted 0.1.0 APK directory")
    ap.add_argument("--so", type=Path, required=True, help="arm64 liblughnasadh.so")
    ap.add_argument("--ref-apk", type=Path, help="Original APK for compression map / enginelist nodes")
    ap.add_argument("--out-dir", type=Path, required=True, help="Staging directory")
    ap.add_argument("--apk", type=Path, required=True, help="Output unsigned APK path")
    args = ap.parse_args()

    import shutil

    if args.out_dir.exists():
        shutil.rmtree(args.out_dir)
    shutil.copytree(args.skeleton, args.out_dir, ignore=shutil.ignore_patterns("META-INF"))
    (args.out_dir / "META-INF").mkdir(exist_ok=True)
    shutil.rmtree(args.out_dir / "META-INF", ignore_errors=True)

    so_dst = args.out_dir / "lib/arm64-v8a/liblughnasadh.so"
    so_dst.parent.mkdir(parents=True, exist_ok=True)
    so_dst.write_bytes(args.so.read_bytes())
    so_dst.chmod(0o755)

    ref_eng = (args.skeleton / "res/xml/enginelist.xml").read_bytes()
    eng = rebuild_enginelist_from_reference(ref_eng, ENGINE_NAME)
    (args.out_dir / "res/xml/enginelist.xml").write_bytes(eng)

    man = patch_manifest_version_name(
        (args.skeleton / "AndroidManifest.xml").read_bytes(), VERSION_NAME
    )
    (args.out_dir / "AndroidManifest.xml").write_bytes(man)

    write_apk(args.out_dir, args.apk, args.ref_apk)
    assert ENGINE_NAME.encode() in eng
    print("enginelist OK:", ENGINE_NAME)


if __name__ == "__main__":
    main()
