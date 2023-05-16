#!/bin/bash
# Script to recursively check if 2 folders are similar, including sha256sum of all files. Used for testing multifile torrents. 

if [ "$#" -ne 2 ]; then
    echo "Usage: $0 <folder1> <folder2>"
    exit 1
fi

folder1="$1"
folder2="$2"

if [ ! -d "$folder1" ] || [ ! -d "$folder2" ]; then
    echo "Both arguments must be directories"
    exit 1
fi

folder1_hashes=$(mktemp)
folder2_hashes=$(mktemp)

find "$folder1" -type f -exec sha256sum {} + | sed "s|$folder1||" | sort -k 2 > "$folder1_hashes"
find "$folder2" -type f -exec sha256sum {} + | sed "s|$folder2||" | sort -k 2 > "$folder2_hashes"

diff_result=$(diff -y --suppress-common-lines "$folder1_hashes" "$folder2_hashes")

if [ -z "$diff_result" ]; then
    echo "The folders are identical"
else
    echo "The folders are different"
    echo "Differences:"
    echo "$diff_result"
fi

rm "$folder1_hashes" "$folder2_hashes"
