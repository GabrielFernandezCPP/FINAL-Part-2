#!/bin/bash
# make_map.sh

# This script copies directories from the maps directory to the current directory.

for arg in "$@"; do
#  echo "Argument: $arg"
  bash copy_dir.sh maps/"$arg" .

done