# Measure first

Making a program use several cores takes work, and it only pays off for the
part of the program that takes the time. This chapter measures one time step
of the simulation, piece by piece, at several system sizes, and then
introduces the two things the next chapters build on: cores and threads.

## Timing one step

The program `step_split.cpp` builds a box of LJ atoms with the engine's own
builder, at density 0.8 as on the home page, and times three things for each
size:

- **The force step:** one call of `ForceCalculator::calculateForces`, the
  force step of {numref}`md-loop-figure`.
- **The energy evaluation:** one call of `ForceCalculator::calculateEnergy`.
  The engine's loop, `MolecularDynamics::runSimulation`, also computes the
  potential energy every step, to report it.
- **Everything else in a step:** integration, thermostat, periodic wrapping.
  To time these alone, the program runs the engine's loop with an energy
  function that does no work at all:

```{literalinclude} examples/step_split.cpp
:language: cpp
:lines: 25-32
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

At N = 4096 a step takes 37.5 ms. The force step and the energy evaluation
take 17.8 and 18.3 ms; everything else takes 0.035 ms, about 0.1% of the
step. Nearly all of the time is in the pair loop.

The energy evaluation costs as much as the force step because it runs the
same pair loop: `LJPotential::calculateEnergy` calls the force kernel and
keeps only the energy. Computing the energy together with the forces would
halve the step time without any parallel work. This module leaves that alone
and makes the pair loop itself faster, which helps both.

## How the time grows with N

{numref}`par-time-vs-n-figure` plots the same numbers on logarithmic axes,
where a time that grows as $N^k$ is a straight line of slope $k$.

```{figure} ../figures/par_time_vs_n.svg
:name: par-time-vs-n-figure
:width: 600px

Time per call against the number of atoms, both axes logarithmic. Dotted
lines show how a time proportional to $N^2$ and to $N$ would grow.
```

The pair loop follows the $N^2$ line. Every atom is compared with every other
atom, which is $N(N-1)/2$ pairs, so doubling N makes four times as many
pairs. A straight-line fit through the points from N = 729 to 4096 gives a
slope of 1.85. Everything else grows as $N$ (fitted slope 1.02): it touches
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

:::{dropdown} 1. At N = 4096, what fraction of a step would remain if the pair loops took no time at all?
About 0.1%: 0.035 ms of 37.5 ms. Speeding up anything other than the pair
loops cannot make a step noticeably faster.
:::

:::{dropdown} 2. The force step takes 17.8 ms at N = 4096. Using the N² rule, what would you expect at N = 8192?
About four times as long, roughly 70 ms: twice the atoms means four times the
pairs.
:::

:::{dropdown} 3. Why does the program report the fastest of several runs, not their average?
Other programs and the operating system can only slow a run down, never speed
it up. The fastest run is the closest to the cost of the code itself; an
average mixes in those outside delays.
:::

## Next

The pair loop is the target. [Threads and races](02_threads_and_races.md)
splits it across threads, and shows the mistake that splitting it the obvious
way makes.
