#!/bin/bash
# cone.sh

# This script moves the contents of the first directory to the second directory's path in path.txt
# and then deletes the first directory.

DIR_A="$1"
DIR_B="$2"

bash copy_content.sh $DIR_A $(cat $DIR_B/path.txt)
rm -r $DIR_A
