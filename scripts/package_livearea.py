"""Validate and package this title's artwork while preserving its working executable."""
import hashlib
from pathlib import Path
import shutil
import struct
import zipfile
import xml.etree.ElementTree as ET

PROJECT = Path(__file__).resolve().parents[1]
ART = PROJECT / "extras/livearea"
VPK = PROJECT / "build/amazing_alex_vita.vpk"
FILES = {
    "icon0.png": "sce_sys/icon0.png",
    "pic0.png": "sce_sys/pic0.png",
    "bg0.png": "sce_sys/livearea/contents/bg0.png",
    "startup.png": "sce_sys/livearea/contents/startup.png",
    "template.xml": "sce_sys/livearea/contents/template.xml",
}
SIZES = {"icon0.png": (128, 128), "pic0.png": (960, 544),
         "bg0.png": (840, 500), "startup.png": (280, 158)}


def validate_art():
    for name, size in SIZES.items():
        data = (ART / name).read_bytes()
        assert data[:8] == b"\x89PNG\r\n\x1a\n", name
        assert struct.unpack_from(">II", data, 16) == size, name
        assert data[24:26] == bytes((8, 3)), f"{name}: expected indexed PNG-8"
        assert len(data) < 420 * 1024, name
    template = (ART / "template.xml").read_bytes()
    assert len(template) < 32768
    root = ET.fromstring(template)
    assert root.tag == "livearea" and root.attrib["style"] == "a1"
    assert not root.findall(".//target"), "LiveArea has no web actions"
    for node in root.findall(".//image") + root.findall(".//startup-image"):
        assert (ART / node.text.strip()).is_file()


def package():
    validate_art()
    temporary = VPK.with_suffix(".vpk.tmp")
    replacements = {target: (ART / source).read_bytes() for source, target in FILES.items()}
    previous_hash = hashlib.sha256(VPK.read_bytes()).hexdigest()
    backup = PROJECT / "analysis/livearea" / ("before-" + previous_hash[:12] + ".vpk")
    backup.parent.mkdir(parents=True, exist_ok=True)
    if not backup.exists():
        shutil.copyfile(VPK, backup)
    with zipfile.ZipFile(VPK) as original:
        assert b"ALEX00001\0" in original.read("sce_sys/param.sfo")
        assert b"Amazing Alex HD\0" in original.read("sce_sys/param.sfo")
        expected_eboot = hashlib.sha256(original.read("eboot.bin")).hexdigest()
        assert expected_eboot == hashlib.sha256((PROJECT / "build/eboot.bin").read_bytes()).hexdigest()
        expected_sfo = original.read("sce_sys/param.sfo")
        with zipfile.ZipFile(temporary, "w", compression=zipfile.ZIP_DEFLATED) as updated:
            for item in original.infolist():
                updated.writestr(item, replacements.pop(item.filename, None) or original.read(item.filename))
            for name, data in replacements.items():
                updated.writestr(name, data)
    with zipfile.ZipFile(temporary) as check:
        assert check.testzip() is None
        assert hashlib.sha256(check.read("eboot.bin")).hexdigest() == expected_eboot
        assert check.read("sce_sys/param.sfo") == expected_sfo
        assert not any(n.endswith((".so", ".apk")) or n.startswith(("data/", "assets/")) for n in check.namelist())
        for source, target in FILES.items():
            assert check.read(target) == (ART / source).read_bytes()
    temporary.replace(VPK)
    print(f"Updated LiveArea in {VPK}")
    print(f"Preserved game executable SHA256: {expected_eboot}")


if __name__ == "__main__":
    package()
