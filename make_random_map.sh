#!/bin/bash

# Wrapper script: randomize_args.sh

# Check if any arguments are given
if [ "$#" -eq 0 ]; then
    echo "Usage: $0 arg1 [arg2 ... argN]"
    exit 1
fi

# Shuffle the arguments using `shuf`
shuffled_args=$(printf "%s\n" "$@" | shuf)

# Call make_maps.sh with shuffled arguments
bash make_map.sh $shuffled_args
