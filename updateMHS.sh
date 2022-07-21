#!/bin/bash
IFS=$'\n' FILES=$(find .. -name "MHS.h")
ME="../simpfs/MHS.h"
for filename in $FILES
do
 if [[ "$filename" != "$ME" ]]; then
 	echo "<SHELL> Copying to $filename"
 	cp "$ME" "$filename"
 else
 	echo "Found ourselves!"
 fi
done
