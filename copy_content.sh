#!/bin/bash
# move.sh

# This script copies the contents of a source directory to a destination directory.

SRC_DIR="$1"
DST_DIR="$2"
FORCE="$3"

if [ ! -d "$SRC_DIR" ]; then
  echo "Error: Source directory '$SRC_DIR' does not exist."
  exit 1
fi

if [ -z "$DST_DIR" ]; then
  echo "Error: Destination directory not specified."
  exit 1
fi

mkdir -p "$DST_DIR"

copy_file() {
  local src="$1"
  local dest="$2"

  if [ "$FORCE" == "--force" ] || [ ! -e "$dest" ]; then
    mkdir -p "$(dirname "$dest")"
    cp -r "$src" "$dest"
  fi
}

export -f copy_file
export FORCE

find "$SRC_DIR" -type f | while read -r file; do
  rel_path="${file#$SRC_DIR/}"
  dest_path="$DST_DIR/$rel_path"
  copy_file "$file" "$dest_path"
done

# echo "Copy complete."
