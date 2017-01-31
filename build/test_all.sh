#!/bin/bash

files=$(find shrtool -maxdepth 1 -name test_\*)

for f in $files; do
    $f
    if [ $? != 0 ]; then
        echo "Error occurs. Stopped"
        break
    fi
done

