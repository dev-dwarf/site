# bit-order-comparison.gnu
# Fixed version – uses inline data with plot '-'

set terminal svg size 600,600 background "white" font "Arial"
set border 3
set tics nomirror
set xtics 1
set ytics 1

set xlabel "8\\*(byte index) + bit value"
set ylabel "bit number" rotate by 90

set xrange [-1:16]
set yrange [-1:16]

# Styles
set style line 1 lc rgb "#0066cc" lt 1 lw 3 pt 7 ps 0.6
set style line 2 lc rgb "#228B22" lt 1 lw 3 pt 7 ps 0.6
set style line 3 lc rgb "#e67e22" lt 2 lw 3 pt 7 ps 0.6
set style line 4 lc rgb "#8e44ad" lt 2 lw 3 pt 7 ps 0.6

# Byte boundary line
set arrow from 8, graph 0 to 8, graph 1 nohead lt 1 lc rgb "grey" lw 1 dt 2

set bmargin 5
set key outside below

plot \
    '-' with points ls 1 title "LE, LSB0", \
    '-' with points ls 2 title "BE, MSB0", \
    '-' with points ls 3 title "LE, MSB0", \
    '-' with points ls 4 title "BE, LSB0"

0  0
1  1
2  2
3  3
4  4
5  5
6  6
7  7
8  8
9  9
10 10
11 11
12 12
13 13
14 14
15 15
e

0 15
1 14
2 13
3 12
4 11
5 10
6  9
7  8
8  7
9  6
10 5
11 4
12 3
13 2
14 1
15 0
e

# LE + MSB0 (zigzag: low byte MSB-first, high byte MSB-first)
0  7
1  6
2  5
3  4
4  3
5  2
6  1
7  0
8 15
9 14
10 13
11 12
12 11
13 10
14  9
15  8
e

# BE + LSB0 (zigzag: low byte LSB-first, high byte LSB-first)
0  8
1  9
2 10
3 11
4 12
5 13
6 14
7 15
8  0
9  1
10 2
11 3
12 4
13 5
14 6
15 7
e

