#!/usr/bin/env bash
set -euo pipefail

TOMB_COMMON_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
TOMB_REPO_ROOT="$(cd -- "$TOMB_COMMON_DIR/.." && pwd)"

: "${DOSBOX_TOMB_SRC:=/tmp/tr1-3dfx}"
: "${DOSBOX_TOMB_STAGE_ROOT:=/tmp/tr1-run}"
: "${DOSBOX_TOMB_GAME_DIR:=TOMBRAID}"
: "${DOSBOX_TOMB_ISO:=tr1disc01.iso}"
: "${DOSBOX_TOMB_EXE:=TOMB.EXE}"

tomb_require_source_tree() {
  if [[ ! -d "$DOSBOX_TOMB_SRC" ]]; then
    echo "Missing Tomb Raider source tree: $DOSBOX_TOMB_SRC" >&2
    return 1
  fi

  if [[ ! -d "$DOSBOX_TOMB_SRC/$DOSBOX_TOMB_GAME_DIR" ]]; then
    echo "Missing game directory: $DOSBOX_TOMB_SRC/$DOSBOX_TOMB_GAME_DIR" >&2
    return 1
  fi

  if [[ ! -f "$DOSBOX_TOMB_SRC/$DOSBOX_TOMB_ISO" ]]; then
    echo "Missing Tomb Raider ISO: $DOSBOX_TOMB_SRC/$DOSBOX_TOMB_ISO" >&2
    return 1
  fi
}

tomb_prepare_stage() {
  local stage_dir

  tomb_require_source_tree
  mkdir -p "$DOSBOX_TOMB_STAGE_ROOT"
  stage_dir="$(mktemp -d "$DOSBOX_TOMB_STAGE_ROOT/tomb.XXXXXX")"
  cp -a "$DOSBOX_TOMB_SRC/." "$stage_dir/"

  if [[ -f "$stage_dir/$DOSBOX_TOMB_GAME_DIR/glide2x.ovl" ]]; then
    mv "$stage_dir/$DOSBOX_TOMB_GAME_DIR/glide2x.ovl" "$stage_dir/$DOSBOX_TOMB_GAME_DIR/glide2x.ovl.disabled"
  fi
  if [[ -f "$stage_dir/glide2x.ovl" ]]; then
    mv "$stage_dir/glide2x.ovl" "$stage_dir/glide2x.ovl.disabled"
  fi

  printf '%s\n' "$stage_dir"
}

tomb_dosbox_args() {
  local stage_dir="$1"

  printf '%s\n' \
    -c "mount c $stage_dir" \
    -c "imgmount d $stage_dir/$DOSBOX_TOMB_ISO -t iso" \
    -c "c:" \
    -c "cd $DOSBOX_TOMB_GAME_DIR" \
    -c "$DOSBOX_TOMB_EXE"
}
