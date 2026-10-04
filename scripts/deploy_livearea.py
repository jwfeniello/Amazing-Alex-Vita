"""Install this title's LiveArea assets and cached artwork with verified backups."""
import argparse
import hashlib
import io
import json
import os
import time
import zipfile
from ftplib import error_perm

from package_livearea import FILES, PROJECT, VPK, validate_art
from vita import TITLE_ID, command, connection, remote_hash, timestamp


def deploy(host):
    validate_art()
    vpk = VPK
    with zipfile.ZipFile(vpk) as package:
        assert package.testzip() is None
        payloads = {name: package.read(name) for name in FILES.values()}
        assert b"ALEX00001\0" in package.read("sce_sys/param.sfo")
        executable_hash = hashlib.sha256(package.read("eboot.bin")).hexdigest()
    tag = timestamp()
    output = PROJECT / "analysis/hardware" / (tag + "-livearea")
    output.mkdir(parents=True)
    plans = []
    for name, data in payloads.items():
        plans.append((f"/ux0:/app/{TITLE_ID}/{name}", data, "app/" + name))
        if name != "sce_sys/param.sfo":
            relative = name.removeprefix("sce_sys/")
            plans.append((f"/ur0:/appmeta/{TITLE_ID}/{relative}", data, "cache/" + relative))
    # Leave a complete installer ready if the shell retains an old title/icon.
    installer = "/ux0:/Amazing Alex HD.vpk"
    plans.append((installer, vpk.read_bytes(), "installer-before.vpk"))
    record = {"host": host, "title_id": TITLE_ID, "eboot_sha256": executable_hash,
              "installer": installer, "files": [], "status": "backing_up"}
    record_path = output / "deployment.json"
    with connection(host) as ftp:
        assert remote_hash(ftp, f"/ux0:/app/{TITLE_ID}/eboot.bin") == executable_hash, "VPK must contain the verified installed game executable"
        for remote, data, name in plans:
            backup = output / name
            backup.parent.mkdir(parents=True, exist_ok=True)
            original = io.BytesIO()
            exists = True
            try:
                ftp.retrbinary("RETR " + remote, original.write)
            except error_perm as exc:
                if not str(exc).startswith("550"):
                    raise
                exists = False
            if exists:
                backup.write_bytes(original.getvalue())
            record["files"].append({"remote": remote, "sha256": hashlib.sha256(data).hexdigest(),
                                    "had_original": exists, "backup": name if exists else None,
                                    "previous_sha256": hashlib.sha256(original.getvalue()).hexdigest() if exists else None,
                                    "remote_backup": remote + ".before-" + tag})
    record_path.write_text(json.dumps(record, indent=2) + "\n")
    command(host, "destroy")
    with connection(host) as ftp:
        # This title's first VPK did not include artwork. Create its directories
        # in both the installed app and shell cache before staging the new files.
        for base in (f"/ux0:/app/{TITLE_ID}/sce_sys", f"/ur0:/appmeta/{TITLE_ID}"):
            for directory in (base, base + "/livearea", base + "/livearea/contents"):
                try:
                    ftp.mkd(directory)
                except error_perm:
                    ftp.cwd(directory)
        for (remote, data, _), entry in zip(plans, record["files"]):
            temporary = remote + ".upload"
            for attempt in range(3):
                try:
                    ftp.storbinary("STOR " + temporary, io.BytesIO(data))
                    break
                except error_perm as exc:
                    if not str(exc).startswith("550") or attempt == 2:
                        raise
                    time.sleep(2)
            assert remote_hash(ftp, temporary) == entry["sha256"], remote
        record["status"] = "staged_and_verified"
        record_path.write_text(json.dumps(record, indent=2) + "\n")
        promoted = []
        try:
            for entry in record["files"]:
                remote = entry["remote"]
                if entry["had_original"]:
                    ftp.rename(remote, entry["remote_backup"])
                try:
                    ftp.rename(remote + ".upload", remote)
                except Exception:
                    if entry["had_original"]:
                        ftp.rename(entry["remote_backup"], remote)
                    raise
                promoted.append(entry)
                assert remote_hash(ftp, remote) == entry["sha256"], remote
                print("Verified " + remote, flush=True)
        except Exception:
            for entry in reversed(promoted):
                ftp.delete(entry["remote"])
                if entry["had_original"]:
                    ftp.rename(entry["remote_backup"], entry["remote"])
            record["status"] = "rolled_back"
            record_path.write_text(json.dumps(record, indent=2) + "\n")
            raise
        assert remote_hash(ftp, f"/ux0:/app/{TITLE_ID}/eboot.bin") == executable_hash
    record["status"] = "installed_and_verified"
    record_path.write_text(json.dumps(record, indent=2) + "\n")
    print(f"LiveArea installed; backups: {output}")
    print("Close the old LiveArea page and open the game's bubble again.")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    config = PROJECT / "analysis/vita-connection.json"
    host = json.loads(config.read_text())["host"] if config.is_file() else os.environ.get("PSVITAIP")
    parser.add_argument("--host", default=host, required=not host)
    deploy(parser.parse_args().host)
