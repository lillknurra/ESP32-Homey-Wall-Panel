#!/usr/bin/env python3
"""Patch116I: offline-only, static GitHub Pages manifest creator and validator.

Never signs images, verifies cryptographic signatures, uploads or publishes anything.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import sys
from urllib.parse import urlsplit

SCHEMA = 1
MAX_BODY = 2048
MAX_IMAGE = 0x400000
MODEL = 'waveshare-esp32-s3-touch-lcd-4b-n16r8'
PROJECT = 'esp32_homey_wall_panel'
RELEASE_PREFIX = 'https://github.com/lillknurra/ESP32-Homey-Wall-Panel/releases/download/'
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
    tag = ('panel-beta-v' if channel == 'beta' else 'panel-v') + version
    return RELEASE_PREFIX + tag + '/' + _image_name(model, version)


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
    if urlsplit(url).scheme != 'https' or urlsplit(url).hostname != 'github.com':
        raise ValueError('release URL must use HTTPS and expected GitHub host')
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


def create(image_path, model, channel, version, release_sequence, notes):
    image_path = Path(image_path)
    if not image_path.is_file() or image_path.is_symlink():
        raise ValueError('signed-image file must exist and must not be symlink')
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
    make = sub.add_parser('create', help='create local manifest AFTER independently verifying firmware signature')
    make.add_argument('--signed-image', required=True)
    make.add_argument('--model', default=MODEL)
    make.add_argument('--channel', choices=['stable', 'beta'], default='stable')
    make.add_argument('--version', required=True)
    make.add_argument('--release-sequence', required=True, type=int)
    make.add_argument('--notes', required=True)
    make.add_argument('--out', required=True)
    make.add_argument('--operator-verified-signature', action='store_true',
                      help='operator confirms signature and verification were performed separately; this flag is NOT proof')
    args = parser.parse_args(argv)
    try:
        if args.action == 'validate':
            x = load_validate(args.manifest)
            print('VALID: schema=1 model=' + x['hardware_model'] + ' channel=' + x['channel'] +
                  ' version=' + x['version'] + ' (not proof of signing or publication)')
        else:
            if not args.operator_verified_signature:
                raise ValueError('operator must independently verify signature before generating release manifest')
            blob = create(args.signed_image, args.model, args.channel, args.version,
                          args.release_sequence, args.notes)
            target = Path(args.out).expanduser().resolve()
            if target.exists():
                raise ValueError('output already exists; refusing overwrite')
            target.parent.mkdir(parents=True, exist_ok=True)
            with target.open('xb') as out:
                out.write(blob)
            print('DRAFT_MANIFEST_CREATED: ' + str(target))
            print('Not published, not cryptographically verified by this script; confirm SHA/size/signature independently.')
    except (OSError, ValueError) as e:
        print('FAIL_CLOSED: ' + str(e), file=sys.stderr)
        return 2
    return 0


if __name__ == '__main__':
    sys.exit(main())
