Creating LJ particle box: N=4096, rho=0.8, L=17.2355 Å
Placed 4096 particles on cubic lattice

N = 4096   rho = 0.80   L = 17.235   cutoff = 2.5   reps = 7
OpenMP: yes  (default threads = 8)

backend               ms/call    vs ref           energy      max|df|      max rel
reference              35.154     1.00x -20105.493949019            -            -
serial-triangular      15.370     2.29x -20105.493949019    1.137e-13    1.490e-11
serial-gather          28.488     1.23x -20105.493949019    1.137e-13    1.490e-11
std::thread             6.714     5.24x -20105.493949019    1.137e-13    1.490e-11
openmp-gather           6.044     5.82x -20105.493949019    1.137e-13    1.490e-11
openmp-private          3.786     9.29x -20105.493949019    1.279e-13    1.158e-11

Best serial kernel: serial-triangular at 15.370 ms.
Speedup measured against THAT (the number you may honestly quote):
    std::thread        2.29x   (vs reference it would look like 5.24x)
    openmp-gather      2.54x   (vs reference it would look like 5.82x)
    openmp-private     4.06x   (vs reference it would look like 9.29x)

Note: std::thread and openmp-gather use the GATHER form, which does
~2x the arithmetic of the triangular kernel -- threading has to win
that back before showing any profit. openmp-private keeps the
triangular loop and gives each thread a private force buffer, so it
avoids that penalty and is the fastest CPU kernel here. It does not
port to a GPU (see lesson 5). Details in docs/learn-parallel/07.

