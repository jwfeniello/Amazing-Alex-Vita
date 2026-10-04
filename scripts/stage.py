"""Stage this game's VPK and private data over FTP, without installing a bubble."""
import argparse
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
from zipfile import ZipFile
from ftplib import error_perm
from vita import connection, remote_hash, PROJECT

def ensure_dir(ftp, path):
    try:
        ftp.mkd(path)
    except error_perm:
        ftp.cwd(path)  # Only accept 'already exists' if the directory is usable.

def stage(host, package):
    data = PROJECT / "dist/data/amazingalex"
    with ZipFile(package) as z:
        if any(n.startswith(('assets/', 'data/')) or n.endswith(('.so', '.apk')) for n in z.namelist()):
            raise ValueError("The VPK must contain only the loader, metadata and port assets.")
        if b'ALEX00001' not in z.read('sce_sys/param.sfo'):
            raise ValueError("Unexpected VPK title ID")
    if not (data / 'libamazingalex.so').is_file():
        raise FileNotFoundError("Run prepare_data.py first")
    native_sha = hashlib.sha256((data / 'libamazingalex.so').read_bytes()).hexdigest()
    if native_sha != 'a0e65a7bc06421c585b42a3bcb0d27b1dc53414bc8a03b7e54b6c5f074adf14b':
        raise ValueError('Unexpected native game library')
    with connection(host) as ftp:
        remote_vpk = '/ux0:/Amazing Alex HD.vpk'
        with package.open('rb') as stream:
            ftp.storbinary('STOR ' + remote_vpk, stream, 65536)
        vpk_sha = hashlib.sha256(package.read_bytes()).hexdigest()
        if remote_hash(ftp, remote_vpk) != vpk_sha:
            raise RuntimeError('VPK readback failed')
        print('VPK uploaded and verified: ' + remote_vpk, flush=True)
        remote_root = '/ux0:/data/amazingalex'
        ensure_dir(ftp, remote_root)
        for folder in sorted((p for p in data.rglob('*') if p.is_dir()), key=lambda p: len(p.parts)):
            ensure_dir(ftp, remote_root + '/' + folder.relative_to(data).as_posix())
        files = sorted(p for p in data.rglob('*') if p.is_file())
        transferred = 0
        for index, path in enumerate(files, 1):
            remote = remote_root + '/' + path.relative_to(data).as_posix()
            with path.open('rb') as stream:
                ftp.storbinary('STOR ' + remote, stream, 65536)
            ftp.voidcmd('TYPE I')
            if ftp.size(remote) != path.stat().st_size:
                raise RuntimeError('Remote size mismatch: ' + remote)
            transferred += path.stat().st_size
            if index % 50 == 0 or index == len(files):
                print(f'Data: {index}/{len(files)} files; {transferred / 1048576:.1f} MiB', flush=True)
        if remote_hash(ftp, remote_root + '/libamazingalex.so') != native_sha:
            raise RuntimeError('Native library readback failed')
    record = {'host': host, 'vpk': remote_vpk, 'vpk_sha256': vpk_sha,
              'data': remote_root, 'files': len(files), 'bytes': transferred,
              'native_sha256': native_sha, 'time': datetime.now(timezone.utc).isoformat()}
    report = PROJECT / 'analysis/staging.json'
    report.parent.mkdir(parents=True, exist_ok=True)
    report.write_text(json.dumps(record, indent=2)+'\n')
    print('All files staged. Install the VPK in VitaShell to register ALEX00001.', flush=True)

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--host', required=True)
    local_build = PROJECT / 'build/amazing_alex_vita.vpk'
    default_vpk = local_build if local_build.is_file() else PROJECT / 'releases/Amazing-Alex-Vita-v0.4.vpk'
    parser.add_argument('--vpk', type=Path, default=default_vpk)
    args = parser.parse_args()
    stage(args.host, args.vpk)
