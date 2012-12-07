#!/bin/sh

for f in *.dot; do
	echo $f
	dot -Tpng -o $f.png $f
done

