#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"

HOST="${DE10_HOST:-}"
USER_NAME="${DE10_USER:-root}"
RBF_FILE="${DE10_RBF:-$ROOT_DIR/output/de10/bitstream/SpinalVoodoo_de10.rbf}"
GLIDE_LIB="${DE10_GLIDE_LIB:-$ROOT_DIR/emu/glide/glide2x/sst1/lib/de10/libglide2x.so.2}"
LAUNCHER="${DE10_LAUNCHER:-$ROOT_DIR/scripts/run-dosboxx-de10-glide}"
CONF_FILE="${DE10_CONF:-$ROOT_DIR/scripts/dosboxx-de10-glide.conf}"
REMOTE_PREFIX="${DE10_REMOTE_PREFIX:-/opt/spinalvoodoo}"
BUNDLE_ID="${DE10_BUNDLE_ID:-$(date +%Y%m%d-%H%M%S)}"
MANIFEST_FILE=""
MANIFEST_LINES=()

cleanup() {
  if [[ -n "$MANIFEST_FILE" && -f "$MANIFEST_FILE" ]]; then
    rm -f "$MANIFEST_FILE"
  fi
}

trap cleanup EXIT

file_hash() {
  sha256sum "$1" | cut -d ' ' -f 1
}

append_manifest() {
  local hash_value="$1"
  local relative_path="$2"

  MANIFEST_LINES+=("${hash_value}  ${relative_path}")
}

copy_verified() {
  local source_path="$1"
  local remote_path="$2"
  local relative_path="$3"
  local mode="${4:-}"
  local hash_value
  local remote_tmp

  hash_value="$(file_hash "$source_path")"
  remote_tmp="${remote_path}.tmp.$$"

  scp "$source_path" "${REMOTE_TARGET}:${remote_tmp}"
  ssh "$REMOTE_TARGET" "actual=\$(sha256sum '$remote_tmp' | cut -d ' ' -f 1); test \"\$actual\" = '$hash_value'; mv -f '$remote_tmp' '$remote_path'"

  if [[ -n "$mode" ]]; then
    ssh "$REMOTE_TARGET" "chmod '$mode' '$remote_path'"
  fi

  append_manifest "$hash_value" "$relative_path"
}

usage() {
  cat <<'EOF'
Usage: scripts/deploy-de10.sh --host <host> [--user <user>] [--rbf <file>] [--remote-prefix <dir>] [--bundle-id <id>]

Deploy steps:
  1) Program FPGA with scripts/program-de10
  2) Upload runtime bundle (FPGA image, glide lib, launcher, config)
  3) Update current symlink to new bundle
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --host)
      HOST="$2"
      shift 2
      ;;
    --user)
      USER_NAME="$2"
      shift 2
      ;;
    --rbf)
      RBF_FILE="$2"
      shift 2
      ;;
    --remote-prefix)
      REMOTE_PREFIX="$2"
      shift 2
      ;;
    --bundle-id)
      BUNDLE_ID="$2"
      shift 2
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      echo "Unknown argument: $1" >&2
      usage
      exit 1
      ;;
  esac
done

if [[ -z "$HOST" ]]; then
  echo "Missing required --host" >&2
  usage
  exit 1
fi

if [[ ! -f "$RBF_FILE" ]]; then
  echo "RBF file not found: $RBF_FILE" >&2
  exit 1
fi

REMOTE_TARGET="${USER_NAME}@${HOST}"
REMOTE_RELEASE_DIR="${REMOTE_PREFIX}/releases/${BUNDLE_ID}"

echo "[de10] Programming FPGA image"
"$ROOT_DIR/scripts/program-de10" --host "$HOST" --user "$USER_NAME" --rbf "$RBF_FILE"

echo "[de10] Creating remote release directories"
ssh "$REMOTE_TARGET" "mkdir -p '$REMOTE_RELEASE_DIR/bin' '$REMOTE_RELEASE_DIR/etc' '$REMOTE_RELEASE_DIR/lib' '$REMOTE_RELEASE_DIR/fpga'"

echo "[de10] Uploading FPGA image"
copy_verified "$RBF_FILE" "$REMOTE_RELEASE_DIR/fpga/$(basename "$RBF_FILE")" "fpga/$(basename "$RBF_FILE")"

if [[ -f "$GLIDE_LIB" ]]; then
  echo "[de10] Uploading Glide library"
  copy_verified "$GLIDE_LIB" "$REMOTE_RELEASE_DIR/lib/libglide2x.so.2" "lib/libglide2x.so.2"
else
  echo "[de10] Glide library not found, skipping: $GLIDE_LIB"
fi

if [[ -f "$LAUNCHER" ]]; then
  echo "[de10] Uploading launcher"
  copy_verified "$LAUNCHER" "$REMOTE_RELEASE_DIR/bin/run-dosboxx-de10-glide" "bin/run-dosboxx-de10-glide" 755
else
  echo "[de10] Launcher not found, skipping: $LAUNCHER"
fi

if [[ -f "$CONF_FILE" ]]; then
  echo "[de10] Uploading DOSBox-X config"
  copy_verified "$CONF_FILE" "$REMOTE_RELEASE_DIR/etc/dosboxx-de10-glide.conf" "etc/dosboxx-de10-glide.conf"
else
  echo "[de10] Config not found, skipping: $CONF_FILE"
fi

MANIFEST_FILE="$(mktemp)"
printf '%s\n' "${MANIFEST_LINES[@]}" > "$MANIFEST_FILE"

echo "[de10] Uploading manifest"
copy_verified "$MANIFEST_FILE" "$REMOTE_RELEASE_DIR/manifest.sha256" "manifest.sha256"

echo "[de10] Activating release ${BUNDLE_ID}"
ssh "$REMOTE_TARGET" "ln -sfn '$REMOTE_RELEASE_DIR' '$REMOTE_PREFIX/current'"

echo "[de10] Deployment complete"
echo "[de10] Active bundle: ${REMOTE_PREFIX}/current"
