#!/bin/bash
echo "<!DOCTYPE RCC><RCC version=\"1.0\"><qresource>"

datadir="`pwd`/data"

cd "$datadir"
(find -type f | sed 's/^\.\///g') | while read f
do
    ff="${datadir}/$f"
    echo "<file alias=\"$f\">$ff</file>"
done

echo "</qresource></RCC>"
