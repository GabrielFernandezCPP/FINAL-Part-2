#!/bin/bash
# copy_dir.sh

# This script copies a directory to a new location.

SRC_DIR="$1"
DST_DIR="$2"

if [ ! -d "$SRC_DIR" ]; then
  echo "Error: Source directory does not exist."
  exit 1
fi

mkdir -p "$DST_DIR"
BASENAME=$(basename "$SRC_DIR")
cp -r "$SRC_DIR" "$DST_DIR/"

# echo "Copied '$SRC_DIR' to '$DST_DIR/$BASENAME'"