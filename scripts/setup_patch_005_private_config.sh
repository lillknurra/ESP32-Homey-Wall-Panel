#!/usr/bin/env bash
set -euo pipefail
export GIT_PAGER=cat
export PAGER=cat
export LESS=FRX

if test "$#" -ne 3; then
  printf 'Usage: %s ABSOLUTE_OUTPUT_PATH KEYCHAIN_SERVICE KEYCHAIN_ACCOUNT
' "$0" >&2
  exit 2
fi
OUTPUT_PATH="$1"
SERVICE="$2"
ACCOUNT="$3"
case "$OUTPUT_PATH" in
  /*) ;;
  *) printf 'REFUSED: private config path must be absolute.
' >&2; exit 2 ;;
esac
REPO_ROOT="$(git rev-parse --show-toplevel)"
CANON_REPO="$(cd "$REPO_ROOT" && pwd -P)"
CANON_PARENT="$(cd "$(dirname "$OUTPUT_PATH")" && pwd -P)"
CANON_OUTPUT="$CANON_PARENT/$(basename "$OUTPUT_PATH")"
case "$CANON_OUTPUT" in
  "$CANON_REPO"|"$CANON_REPO"/*) printf 'REFUSED: private config must be outside repository.
' >&2; exit 2 ;;
esac
if test -e "$CANON_OUTPUT"; then
  printf 'REFUSED: target already exists.
' >&2
  exit 2
fi
umask 077
node -e 'const fs=require("node:fs"); const [path,service,account]=process.argv.slice(1); const value={connection:"auto",credential_provider:"macos-keychain",keychain_service:service,keychain_account:account}; fs.writeFileSync(path, JSON.stringify(value,null,2)+"\n", {encoding:"utf8",mode:0o600,flag:"wx"});' "$CANON_OUTPUT" "$SERVICE" "$ACCOUNT"
chmod 600 "$CANON_OUTPUT"
node -e 'const fs=require("node:fs"); const path=process.argv[1]; const value=JSON.parse(fs.readFileSync(path,"utf8")); if(value.credential_provider!=="macos-keychain"||typeof value.keychain_service!=="string"||typeof value.keychain_account!=="string") process.exit(2);' "$CANON_OUTPUT"
printf 'Created validated private config with restrictive permissions outside repository.
'
printf 'No credential value was requested, read, or written.
'
