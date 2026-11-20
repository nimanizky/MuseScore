"""Create lossless, independently downloadable ZIPs and a Windows installer."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import zipfile


def digest(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('source', type=Path)
    parser.add_argument('destination', type=Path)
    args = parser.parse_args()
    files = sorted(args.source.glob('Gamelan_*.sf2'))
    if not files:
        parser.error('No Gamelan soundfonts found')
    args.destination.mkdir(parents=True, exist_ok=True)
    entries = []
    for source in files:
        archive = args.destination / (source.stem + '.zip')
        with zipfile.ZipFile(archive, 'w', compression=zipfile.ZIP_DEFLATED, compresslevel=6) as out:
            out.write(source, source.name)
        with zipfile.ZipFile(archive) as check:
            if check.testzip():
                raise ValueError(f'Archive verification failed: {archive}')
        entries.append(dict(filename=source.name, sha256=digest(source), archive=archive.name,
                            archive_sha256=digest(archive), bytes=source.stat().st_size))
        print(f'{source.name}: {source.stat().st_size // 1048576} -> {archive.stat().st_size // 1048576} MiB', flush=True)
    (args.destination / 'manifest.json').write_text(json.dumps(dict(files=entries), indent=2))
    scripts = Path(__file__).parent
    for name in ('Install-Sound-Pack.cmd', 'Install-Sound-Pack.ps1'):
        shutil.copyfile(scripts / name, args.destination / name)
    (args.destination / 'READ-ME.txt').write_text(
        'Install the Sandikala Windows MSI first, and close MuseScore.\n'
        'Keep ALL ZIP files, manifest.json, and both Install-Sound-Pack files in this folder.\n'
        'Do not extract the soundfont ZIP files. Double-click Install-Sound-Pack.cmd and approve administrator access.\n'
        'Restart MuseScore after installation. Samples are preserved without quality loss.\n')
    with zipfile.ZipFile(args.destination / 'Sound-Pack-Installer.zip', 'w', zipfile.ZIP_DEFLATED) as out:
        for name in ('Install-Sound-Pack.cmd', 'Install-Sound-Pack.ps1', 'manifest.json', 'READ-ME.txt'):
            out.write(args.destination / name, name)
    print('Sound pack complete.', flush=True)


if __name__ == '__main__':
    main()
