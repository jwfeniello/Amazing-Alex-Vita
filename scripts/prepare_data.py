"""Prepare private Amazing Alex HD 1.0.5 assets. Never bundle these in source/VPK."""
import argparse
import hashlib
import io
import json
from pathlib import Path, PurePosixPath
import zipfile

EXPECTED_SO = "a0e65a7bc06421c585b42a3bcb0d27b1dc53414bc8a03b7e54b6c5f074adf14b"

def prepare(apk, output):
    with zipfile.ZipFile(apk) as package:
        native = package.read("lib/armeabi-v7a/libamazingalex.so")
        if hashlib.sha256(native).hexdigest() != EXPECTED_SO:
            raise ValueError("This loader targets the inspected Amazing Alex HD 1.0.5 ARMv7 library.")
        output.mkdir(parents=True, exist_ok=True)
        (output / "libamazingalex.so").write_bytes(native)
        assets = {}
        # Match MyRenderer.readFile: a ZIP takes precedence over a raw asset.
        for item in package.infolist():
            if not item.filename.startswith("assets/") or item.is_dir():
                continue
            rel = PurePosixPath(item.filename)
            if rel.is_absolute() or ".." in rel.parts or ":" in item.filename or "\\" in item.filename:
                raise ValueError("Unsafe asset path")
            raw = package.read(item)
            if item.filename.endswith(".zip"):
                with zipfile.ZipFile(io.BytesIO(raw)) as zipped:
                    first = next((p for p in zipped.infolist() if not p.is_dir()), None)
                    if first is None:
                        raise ValueError("Empty nested asset ZIP")
                    if first.file_size > 32 * 1024 * 1024:
                        raise ValueError("Asset exceeds loader limit")
                    raw = zipped.read(first)
                assets[item.filename[:-4]] = (True, raw)
            elif item.filename not in assets or not assets[item.filename][0]:
                assets[item.filename] = (False, raw)
        for rel, (_, raw) in assets.items():
            path = output / rel
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(raw)
        manifest = {"game": "Amazing Alex HD 1.0.5", "native_sha256": EXPECTED_SO,
                    "assets": len(assets), "asset_bytes": sum(len(v[1]) for v in assets.values()),
                    "apk_sha256": hashlib.sha256(Path(apk).read_bytes()).hexdigest()}
        (output / "port-data.json").write_text(json.dumps(manifest, indent=2)+"\n")
        return manifest

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("apk", type=Path)
    parser.add_argument("--output", type=Path, default=Path(__file__).resolve().parents[1]/"dist/data/amazingalex")
    args = parser.parse_args()
    print(json.dumps(prepare(args.apk, args.output), indent=2))
