#!/bin/bash

bash get_maps.sh "$@"

# Loop backwards through arguments
for (( i=$#; i>1; i-- )); do
    arg1=${!i}
    prev_index=$((i - 1))
    arg2=${!prev_index}
    # echo "bash cone.sh $arg1 $arg2"
    bash connect.sh "$arg1" "$arg2"
done

# The last argument is the first one in the loop
bash copy_content.sh $arg2 "new_map" --force
rm -r "$arg2"

