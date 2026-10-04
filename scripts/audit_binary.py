"""Read-only ELF inventory and checks against the port's compatibility tables."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct


def elf_inventory(path):
    data = path.read_bytes()
    if data[:6] != b"\x7fELF\x01\x01" or struct.unpack_from("<H", data, 18)[0] != 40:
        raise ValueError("Expected ELF32 little-endian ARM")
    shoff = struct.unpack_from("<I", data, 32)[0]
    entsize, count, strings_idx = struct.unpack_from("<HHH", data, 46)
    sections = [struct.unpack_from("<10I", data, shoff + i * entsize) for i in range(count)]
    names = sections[strings_idx]
    strings = data[names[4]:names[4] + names[5]]
    def cstring(blob, offset):
        return blob[offset:blob.index(b"\0", offset)].decode("utf-8", "replace")
    named = {cstring(strings, s[0]): s for s in sections}
    dynsym = named[".dynsym"]
    string_section = sections[dynsym[6]]
    symbols = data[string_section[4]:string_section[4] + string_section[5]]
    imports, exports = [], {}
    for offset in range(dynsym[4], dynsym[4] + dynsym[5], dynsym[9]):
        name, address, size, info, other, section = struct.unpack_from("<IIIBBH", data, offset)
        if not name:
            continue
        name = cstring(symbols, name)
        if section == 0:
            imports.append({"name": name, "weak": info >> 4 == 2})
        else:
            exports[name] = {"address": address, "size": size}
    return {"sha1": hashlib.sha1(data).hexdigest(), "sha256": hashlib.sha256(data).hexdigest(),
            "imports": sorted(imports, key=lambda s: s["name"]), "exports": exports,
            "debug_info": ".debug_info" in named}



def audit(path, project):
    inventory = elf_inventory(path)
    source = (project / "source/dynlib.c").read_text()
    mapped_names = set(re.findall(r'\{\s*"([^"\n]+)"\s*,', source))
    mappings = dict(re.findall(r'\{\s*"([^"\n]+)"\s*,\s*\(uintptr_t\)&?(\w+)', source))
    inventory["unmapped_imports"] = [v["name"] for v in inventory["imports"] if v["name"] not in mapped_names]
    inventory["placeholder_imports"] = [v["name"] for v in inventory["imports"] if mappings.get(v["name"]) in ("ret0", "ret1")]
    combined = "\n".join(p.read_text() for p in (project / "source").glob("*.c"))
    required = set(re.findall(r'"(Java_com_rovio_[A-Za-z0-9_]+|JNI_OnLoad|_ZN[A-Za-z0-9_]+)"', combined))
    required = {n for n in required if n.startswith(("Java_", "JNI_")) or n in {
        "_ZN13HttpOperation15InformListenersEb", "_ZN13HttpOperation13ThreadCleanupEb",
        "_ZN13HttpOperation10ThreadFuncEb", "_ZN2pf7WebView18isWebViewSupportedEv"}}
    inventory["missing_exports"] = sorted(required - inventory["exports"].keys())
    return inventory


if __name__ == "__main__":
    project = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--so", type=Path, default=project / "dist/data/amazingalex/libamazingalex.so")
    args = parser.parse_args()
    result = audit(args.so, project)
    (project / "analysis").mkdir(exist_ok=True)
    (project / "analysis/binary-audit.json").write_text(json.dumps(result, indent=2)+"\n")
    print(f"{len(result['imports'])} imports; unmapped: {result['unmapped_imports']}; missing exports: {result['missing_exports']}")
    print("Compatibility placeholders still needing runtime validation:", result["placeholder_imports"])
    raise SystemExit(bool(result["unmapped_imports"] or result["missing_exports"]))
