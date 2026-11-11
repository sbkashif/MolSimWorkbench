# Measure first

The plan of this module is to split the pair loop across cores. First, a
check that the pair loop really is where the time goes: making a program use
several cores takes work, and it only pays off for the part of the program
that takes the time. This chapter measures one time step of the simulation,
piece by piece, at several system sizes, and then introduces the two things
the next chapters build on: cores and threads.

## Timing one step

The program `step_split.cpp` builds a box of LJ atoms with the engine's own
builder, at density 0.8, a liquid, and times four things for each
size:

- **A whole step:** the engine's own loop, `MolecularDynamics::runSimulation`,
  run for 50 steps, divided by 50.
- **The force step:** one call of `ForceCalculator::calculateForces`, the
  force step of {numref}`md-loop-figure`.
- **The energy evaluation:** one call of `ForceCalculator::calculateEnergy`.
  The loop reports the potential energy every step, so it needs this number
  too.
- **Everything else in a step:** integration, thermostat, periodic wrapping.
  To time these alone, the program runs the engine's loop with an energy
  function that does no work at all:

```{literalinclude} examples/step_split.cpp
:language: cpp
:start-at: "// An energy function that does no work"
:end-at: "};"
:caption: step_split.cpp (an energy function that does nothing)
```

A run with this plug does every part of a step except the pair loop, so its
time per step is the time of everything else.

Each time is the fastest of several runs, after one warm-up run. The first
run of anything pays one-off costs, such as loading data into the processor's
caches; and the operating system and other programs can only ever make a run
slower, never faster, so the fastest run is the closest to the true cost.
(The builder also prints two progress lines per size; they are left out
below.)

### The first measurement found a waste

The first time this program ran, on an earlier version of the engine, it
printed:

```console
$ ./step_split
     N    step_ms  forces_ms  energy_ms    rest_ms
   125     0.0572     0.0287     0.0282     0.0011
   343     0.4466     0.1889     0.1884     0.0026
   729     1.6732     0.7370     0.7537     0.0055
  1331     4.6842     2.2248     2.2122     0.0121
  2197    11.3442     5.6390     5.4924     0.0194
  4096    37.4713    17.7970    18.3414     0.0351
```

At N = 4096 a step took 37.5 ms: the force step (17.8 ms) **plus** the
energy evaluation (18.3 ms) plus 0.035 ms for everything else. The energy
evaluation cost as much as the force step because it ran the same pair loop
a second time. The pair loop produces the energy and the forces together;
the force step kept the forces and dropped the energy, and then the energy
evaluation ran the loop again and dropped the forces.

The fix needs no parallel programming. The engine now has one call that
returns both, `calculateForcesAndEnergy`, and the loop takes the energy from
the force step it already ran:

```{literalinclude} ../../src/engine/potentials/lj_potential.cpp
:language: cpp
:start-at: "double LJPotential::calculateForcesAndEnergy("
:end-before: "// ======"
:caption: LJPotential::calculateForcesAndEnergy (src/engine/potentials/lj_potential.cpp)
```

`ljComputeGather` is the pair loop. It writes the forces into the
potential's own arrays (`sfx`, `sfy`, `sfz`) and returns the energy; the
`for` loop copies the forces out in the form the rest of the engine uses.
`tailCorrectionEnergy` adds a fixed correction for the pairs beyond the
cutoff, with no loop over pairs. The trajectory does not change: a run before
and after the fix prints the same energies at every step.

### The step now

The same program on today's engine:

```console
$ ./step_split
     N    step_ms  forces_ms  energy_ms    rest_ms
   125     0.0329     0.0287     0.0283     0.0012
   343     0.2433     0.2176     0.2159     0.0028
   729     0.8311     0.7338     0.7311     0.0057
  1331     2.3012     2.1675     2.1770     0.0115
  2197     5.5764     5.3537     5.3602     0.0190
  4096    18.4416    17.4110    17.9478     0.0344
```

The step at N = 4096 takes 18.4 ms, half of before. The energy evaluation
column still costs a full pair loop, but only when called on its own; a step
no longer calls it. What is left is the force step, 17.4 ms, and 0.034 ms of
everything else, about 0.2% of the step. Nearly all of the time is in the
pair loop, and the rest of this module makes that loop faster.

## How the time grows with N

{numref}`par-time-vs-n-figure` plots the step times, before and after the
fix, on logarithmic axes, where a time that grows as $N^k$ is a straight line
of slope $k$.

```{figure} ../figures/par_time_vs_n.svg
:name: par-time-vs-n-figure
:width: 600px

Time per step against the number of atoms, both axes logarithmic. Dotted
lines show how a time proportional to $N^2$ and to $N$ would grow. Computing
the energy in the force pass moves the whole line down by a factor of two; it
does not change its slope.
```

The step follows the $N^2$ line. Every atom is compared with every other
atom, which is $N(N-1)/2$ pairs, so doubling N makes four times as many
pairs. A straight-line fit through the points from N = 729 to 4096 gives a
slope of 1.79. Everything else grows as $N$ (fitted slope 1.04): it touches
each atom a fixed number of times.

The cutoff does not change this. The loop still computes the distance of
every pair before it skips the pairs beyond 2.5σ. Programs that simulate
millions of atoms keep, for each atom, a list of the atoms near it and visit
only those; this engine does not have such a list yet, and this module does
not need one.

## Cores and threads

[A first program](../cpp/01_first_program.md#the-parts-of-a-computer-this-page-uses)
drew the processor as registers plus an execution unit that runs
instructions one after another. A **core** is one such set: its own
registers, and its own unit running its own instructions. A processor with
several cores can run several streams of instructions at the same time. The
machine used here has 10:

```console
$ sysctl -n machdep.cpu.brand_string hw.perflevel0.physicalcpu hw.perflevel1.physicalcpu
Apple M4
4
6
```

4 **performance** cores, which are fast, and 6 **efficiency** cores, which
are slower and use less power.

A program uses several cores through **threads**. A thread is one stream of
instructions within a program; in the words of the C++ reference, the class
`std::thread` "represents a single thread of execution"
([std::thread](https://en.cppreference.com/w/cpp/thread/thread)). Every
program starts with one thread, running `main`, and can start more. The
operating system runs each thread on a core, and all the threads of a
program share its memory ({numref}`par-cores-threads-figure`).

```{figure} ../figures/par_cores_threads.svg
:name: par-cores-threads-figure

One program with four threads. The operating system decides which core runs
each thread (here, as an example, the four performance cores). All threads
read and write the same memory, including the force arrays.
```

Shared memory is what makes threads useful here: every thread can read all
the positions. It is also the danger, which the next chapter shows: two
threads can write the same force at the same time.

:::{admonition} If you know Python
:class: tip
In the standard CPython interpreter, threads do not run Python code in
parallel: the global interpreter lock lets "only one thread execute Python
bytecode at a time"
([glossary: global interpreter lock](https://docs.python.org/3/glossary.html)).
Numerical libraries get around it by doing their heavy work in compiled code,
which is one reason the engine's pair loop is written in C++. (Python 3.13
added an optional build without the lock.)
:::

## Check yourself

:::{dropdown} 1. At N = 4096, what fraction of a step would remain if the pair loop took no time at all?
About 0.2%: 0.034 ms of 18.4 ms. Speeding up anything other than the pair
loop cannot make a step noticeably faster.
:::

:::{dropdown} 2. The force step takes 17.4 ms at N = 4096. Using the N² rule, what would you expect at N = 8192?
About four times as long, roughly 70 ms: twice the atoms means four times the
pairs.
:::

:::{dropdown} 3. The step got twice as fast without any threads. What did the measurement show that pointed to it?
The step time was the sum of the force step and the energy evaluation, and
the two cost the same. Two equal costs for two results of one loop meant the
loop ran twice.
:::

:::{dropdown} 4. Why does the program report the fastest of several runs, not their average?
Other programs and the operating system can only slow a run down, never speed
it up. The fastest run is the closest to the cost of the code itself; an
average mixes in those outside delays.
:::

## Next

The pair loop is the target. [Threads and races](02_threads_and_races.md)
splits it across threads, and shows the mistake that splitting it the obvious
way makes.
