#!/usr/bin/gnuplot

#set palette grey

set xrange [0:512]
set yrange [0:50]

set xlabel "Bytes"
set ylabel "Blocks"

unset key
unset colorbox

set xtics 32
set ytics 1

set term x11

numImages = system('ls -l *.dot | wc -l')

print  numImages

do for [ii=1:numImages-1] {
	set title sprintf("After %d insertions", ii*5)
	plot sprintf("./bild%04d.dot", ii)  matrix with image
#	print sprintf("./bild%04d.dot", ii)
}
