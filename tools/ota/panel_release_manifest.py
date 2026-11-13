#!/usr/bin/env python3
"""Offline-only GitHub Pages release manifest creator and validator.

Creation requires native ESP-IDF Secure Boot v2 signature verification against
an explicitly supplied public key. This tool never signs, uploads or publishes.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess
import sys
from urllib.parse import urlsplit

SCHEMA = 1
MAX_BODY = 2048
MAX_IMAGE = 0x400000
MODEL = 'waveshare-esp32-s3-touch-lcd-4b-n16r8'
PROJECT = 'esp32_homey_wall_panel'
ORIGIN = 'https://lillknurra.github.io/ESP32-Homey-Wall-Panel/'
RELEASE_PREFIX = ORIGIN + 'updates/releases/'
FIELDS = {'schema_version', 'hardware_model', 'project', 'channel',
          'release_sequence', 'version', 'image', 'notes'}
IMAGE_FIELDS = {'size_bytes', 'sha256', 'url'}
VERSION = re.compile(r'(?:0|[1-9][0-9]{0,7})\.(?:0|[1-9][0-9]{0,7})\.(?:0|[1-9][0-9]{0,7})\Z')
ID = re.compile(r'[a-z0-9_-]+\Z')


def _no_duplicate_pairs(pairs):
    result = {}
    for key, value in pairs:
        if key in result:
            raise ValueError('duplicate JSON key: ' + key)
        result[key] = value
    return result


def _is_uint32(x):
    return type(x) is int and 0 <= x <= 0xffffffff


def _text(value, size):
    if not isinstance(value, str) or '\x00' in value:
        return False
    try:
        return len(value.encode('utf-8')) < size
    except UnicodeError:
        return False


def _image_name(model, version):
    return f'esp32_homey_wall_panel_{model}_{version}_signed.bin'


def _url(model, version, channel='stable'):
    return RELEASE_PREFIX + version + '/' + _image_name(model, version)


def release_sequence_for_version(version):
    """Sequence authenticated through the signed ESP app descriptor version."""
    if not isinstance(version, str) or VERSION.fullmatch(version) is None:
        raise ValueError('invalid semantic version for release sequence')
    major, minor, patch = (int(part) for part in version.split('.'))
    if minor > 999 or patch > 999:
        raise ValueError('minor and patch release components must be 0..999')
    sequence = major * 1_000_000 + minor * 1_000 + patch
    if sequence <= 0 or sequence > 0xffffffff:
        raise ValueError('version-derived release sequence is outside uint32 range')
    return sequence


def _app_descriptor(image_path):
    """Read the ESP image's first-segment app descriptor after signature verify."""
    with Path(image_path).open('rb') as image:
        header = image.read(24)
        if len(header) != 24 or header[0] != 0xE9 or not (1 <= header[1] <= 16):
            raise ValueError('invalid ESP application image header')
        if int.from_bytes(header[12:14], 'little') != 9:
            raise ValueError('signed image target is not ESP32-S3')
        segment = image.read(8)
        if len(segment) != 8:
            raise ValueError('truncated first image segment header')
        segment_length = int.from_bytes(segment[4:8], 'little')
        if segment_length < 256 or segment_length > 0x400000 - 32:
            raise ValueError('invalid app descriptor segment size')
        desc = image.read(256)
        if len(desc) != 256 or int.from_bytes(desc[0:4], 'little') != 0xABCD5432:
            raise ValueError('signed image app descriptor is missing or invalid')
        def bounded_ascii(raw, field):
            value = raw.split(b'\0', 1)[0]
            try:
                decoded = value.decode('ascii')
            except UnicodeDecodeError as exc:
                raise ValueError('non-ASCII app descriptor ' + field) from exc
            if not decoded:
                raise ValueError('empty app descriptor ' + field)
            return decoded
        return {
            'version': bounded_ascii(desc[16:48], 'version'),
            'project': bounded_ascii(desc[48:80], 'project'),
            'secure_version': int.from_bytes(desc[4:8], 'little'),
        }


def validate(data):
    if not isinstance(data, dict) or set(data) != FIELDS:
        raise ValueError('exact schema v1 top-level fields are required')
    image = data.get('image')
    if not isinstance(image, dict) or set(image) != IMAGE_FIELDS:
        raise ValueError('image fields missing or unknown')
    if type(data['schema_version']) is not int or data['schema_version'] != SCHEMA:
        raise ValueError('unsupported schema')
    for field, size in [('hardware_model', 64), ('project', 48), ('channel', 12)]:
        if not _text(data[field], size) or not ID.fullmatch(data[field]):
            raise ValueError('invalid ' + field)
    if data['channel'] not in {'stable', 'beta'}:
        raise ValueError('invalid channel')
    if not _text(data['version'], 32) or not VERSION.fullmatch(data['version']):
        raise ValueError('invalid semantic version')
    if not _is_uint32(data['release_sequence']) or data['release_sequence'] == 0:
        raise ValueError('invalid release sequence')
    if data['release_sequence'] != release_sequence_for_version(data['version']):
        raise ValueError('release sequence does not match signed app version policy')
    if not _text(data['notes'], 160) or any(ord(c) < 32 or ord(c) == 127 for c in data['notes']):
        raise ValueError('invalid notes')
    if not _is_uint32(image['size_bytes']) or not (288 <= image['size_bytes'] <= MAX_IMAGE):
        raise ValueError('invalid image size')
    digest = image['sha256']
    if not isinstance(digest, str) or re.fullmatch(r'[0-9a-f]{64}', digest) is None or digest == '0' * 64:
        raise ValueError('invalid image sha256')
    url = image['url']
    if not _text(url, 384) or url != _url(data['hardware_model'], data['version'], data['channel']):
        raise ValueError('invalid or non-canonical release URL')
    if not url.startswith(RELEASE_PREFIX) or urlsplit(url).scheme != 'https' or \
       urlsplit(url).hostname != 'lillknurra.github.io' or urlsplit(url).port is not None or \
       urlsplit(url).query or urlsplit(url).fragment:
        raise ValueError('release URL must use the fixed HTTPS GitHub Pages origin')
    payload = json.dumps(data, ensure_ascii=False, indent=2, sort_keys=True).encode('utf-8') + b'\n'
    if len(payload) > MAX_BODY:
        raise ValueError('manifest exceeds panel byte limit')
    return payload


def load_validate(path):
    raw = Path(path).read_bytes()
    if len(raw) > MAX_BODY or len(raw) == 0 or b'\x00' in raw:
        raise ValueError('manifest byte size or NUL invalid')
    try:
        data = json.loads(raw.decode('utf-8'), object_pairs_hook=_no_duplicate_pairs,
                          parse_constant=lambda _x: (_ for _ in ()).throw(ValueError('invalid numeric token')))
    except (ValueError, UnicodeDecodeError) as e:
        raise ValueError('invalid JSON: ' + str(e)) from e
    validate(data)
    return data


def verify_signed_image(image_path, public_key, espsecure='espsecure'):
    image_path = Path(image_path)
    public_key = Path(public_key)
    if not image_path.is_file() or image_path.is_symlink():
        raise ValueError('signed-image file must exist and must not be symlink')
    if not public_key.is_file() or public_key.is_symlink():
        raise ValueError('verification public key must exist and must not be symlink')
    executable = shutil.which(espsecure)
    if executable is None and Path(espsecure).is_file():
        executable = str(Path(espsecure).resolve())
    if executable is None:
        raise ValueError('ESP-IDF espsecure executable is unavailable')
    try:
        result = subprocess.run(
            [executable, 'verify-signature', '--version', '2', '--keyfile',
             str(public_key.resolve()), str(image_path.resolve())],
            capture_output=True, text=True, timeout=60, check=False)
    except (OSError, subprocess.TimeoutExpired) as exc:
        raise ValueError('native ESP-IDF signature verification could not complete') from exc
    if result.returncode != 0:
        raise ValueError('ESP-IDF Secure Boot v2 signature verification failed')


def create(image_path, model, channel, version, release_sequence, notes,
           public_key=None, espsecure='espsecure'):
    image_path = Path(image_path)
    if public_key is None:
        raise ValueError('an explicit public key is required for cryptographic signature verification')
    verify_signed_image(image_path, public_key, espsecure)
    if image_path.stat().st_size < 288:
        raise ValueError('signed image size is below minimum application image size')
    if image_path.stat().st_size > MAX_IMAGE:
        raise ValueError('signed image exceeds maximum OTA partition capacity')
    desc = _app_descriptor(image_path)
    if desc['project'] != PROJECT or desc['version'] != version:
        raise ValueError('manifest project/version does not match signed app descriptor')
    if release_sequence != release_sequence_for_version(desc['version']):
        raise ValueError('release sequence does not match signed app descriptor version')
    digest = hashlib.sha256()
    total = 0
    with image_path.open('rb') as f:
        while block := f.read(65536):
            digest.update(block)
            total += len(block)
            if total > MAX_IMAGE:
                raise ValueError('signed image exceeds maximum OTA partition capacity')
    data = {
        'schema_version': 1,
        'hardware_model': model,
        'project': PROJECT,
        'channel': channel,
        'release_sequence': release_sequence,
        'version': version,
        'image': {
            'size_bytes': total,
            'sha256': digest.hexdigest(),
            'url': _url(model, version, channel),
        },
        'notes': notes,
    }
    return validate(data)


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest='action', required=True)
    check = sub.add_parser('validate', help='validate an existing local JSON manifest (read-only)')
    check.add_argument('manifest')
    make = sub.add_parser('create', help='create a local manifest after native signature verification')
    make.add_argument('--signed-image', required=True)
    make.add_argument('--public-key', required=True,
                      help='trusted ESP Secure Boot v2 public key; never a private signing key')
    make.add_argument('--espsecure', default='espsecure',
                      help='ESP-IDF espsecure executable (default: espsecure on PATH)')
    make.add_argument('--model', default=MODEL)
    make.add_argument('--channel', choices=['stable', 'beta'], default='stable')
    make.add_argument('--version', required=True)
    make.add_argument('--release-sequence', required=True, type=int)
    make.add_argument('--notes', required=True)
    make.add_argument('--out', required=True)
    args = parser.parse_args(argv)
    try:
        if args.action == 'validate':
            x = load_validate(args.manifest)
            print('VALID: schema=1 model=' + x['hardware_model'] + ' channel=' + x['channel'] +
                  ' version=' + x['version'] + ' (not proof of signing or publication)')
        else:
            blob = create(args.signed_image, args.model, args.channel, args.version,
                          args.release_sequence, args.notes, args.public_key,
                          args.espsecure)
            target = Path(args.out).expanduser().resolve()
            if target.exists():
                raise ValueError('output already exists; refusing overwrite')
            target.parent.mkdir(parents=True, exist_ok=True)
            with target.open('xb') as out:
                out.write(blob)
            print('DRAFT_MANIFEST_CREATED: ' + str(target))
            print('Not published. Signature verified locally with ESP-IDF espsecure.')
    except (OSError, ValueError) as e:
        print('FAIL_CLOSED: ' + str(e), file=sys.stderr)
        return 2
    return 0


if __name__ == '__main__':
    sys.exit(main())
