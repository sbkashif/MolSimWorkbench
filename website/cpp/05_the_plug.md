# The plug: inheritance and virtual functions

[Classes](04_classes.md) ended with one energy function, `LJPotential`,
packaged as a class. The engine needs more than one kind:
[A minimal molecular simulation algorithm](../molsim/architecture.md#the-energy-function-is-a-plug)
described the energy function as a plug, where the force step asks for an
energy and forces, and any energy function that answers fits the same
place. This chapter writes that in C++. A class `Potential` is the socket;
`LJPotential` and a second energy function are two plugs; a small
`ForceCalculator` holds any number of plugs and adds them up, without
knowing which kinds it holds.

The whole program is one file, `plug.cpp`. The sections below show it part
by part.

## The socket: a class with only promises

```{literalinclude} examples/ch05/plug.cpp
:language: cpp
:lines: 7-13
:lineno-start: 7
:caption: plug.cpp (the socket)
```

`Potential` has no data and no formulas. It lists what every energy function
must provide:

- **`virtual double energy(double r) const = 0;`** `virtual` means: the
  version of this function that runs is chosen when the program runs, by the
  kind of object it is called on (the next sections show how). `= 0` means
  `Potential` itself gives no version at all; every plug must supply one. A
  function marked `= 0` is called **pure virtual**.
- **`virtual std::string name() const = 0;`** the same for a name to print.
  `std::string` is the standard library's type for text, from `<string>`.
- **`virtual ~Potential() = default;`** the **destructor**, the function that
  runs when an object is removed, named with a `~`. Making it `virtual`
  ensures that removing a plug through a `Potential` pointer runs the right
  plug's cleanup. `= default` asks the compiler to write the usual one.

A class with a pure virtual function is **abstract**: you cannot make an
object of it, because some of its functions do not exist. You can only make
objects of classes that supply them all.

## Two plugs

```{literalinclude} examples/ch05/plug.cpp
:language: cpp
:lines: 15-36
:lineno-start: 15
:caption: plug.cpp (plug 1)
```

This is the `LJPotential` of [Classes](04_classes.md), with two changes:

- **`: public Potential`** on line 16 makes `LJPotential` **inherit** from
  `Potential`: an `LJPotential` is a kind of `Potential`, and anything that
  can hold a `Potential` can hold an `LJPotential`.
- **`override`** on lines 23 and 30 says "this supplies one of the socket's
  functions". It is a check: if the function does not match one in
  `Potential` exactly, the compiler stops (below).

The second plug is a soft repulsion, a simple energy function used in coarse
models: $U(r) = A\,(1 - r/r_c)^2$ for $r < r_c$, and 0 beyond. It pushes
overlapping particles apart and has no attraction.

```{literalinclude} examples/ch05/plug.cpp
:language: cpp
:lines: 38-57
:lineno-start: 38
:caption: plug.cpp (plug 2)
```

Its data members differ from `LJPotential`'s, and its formula differs, but
it supplies the same two functions with the same parameters, so it fits the
same socket.

## The force step: a list of plugs

```{literalinclude} examples/ch05/plug.cpp
:language: cpp
:lines: 59-80
:lineno-start: 59
:caption: plug.cpp (the force step)
```

`ForceCalculator` keeps its plugs in `potentials_`, a vector of
`std::shared_ptr<Potential>` (line 79). A **pointer** holds the address of
an object, chapter 1's `&r`, kept in a variable. `std::shared_ptr`, from
`<memory>`, is the standard library's pointer that also looks after the
object: the object stays alive as long as some `shared_ptr` points to it, and
is removed when the last one goes away. `p->energy(r)` on line 68 calls
`energy` on the object that `p` points to; `->` is the `.` of chapter 3 for a
pointer.

`energy` (lines 65–71) only knows that each plug is some `Potential`. It
calls `p->energy(r)` on each and adds the results. It never names
`LJPotential` or `SoftRepulsion`, so a third plug written next year would
fit without changing a line of `ForceCalculator`.

```{literalinclude} examples/ch05/plug.cpp
:language: cpp
:lines: 82-93
:lineno-start: 82
:caption: plug.cpp (main)
```

`std::make_shared<LJPotential>(1.0, 1.0, 2.5)` makes an `LJPotential` with
those constructor arguments and returns a `shared_ptr` to it; passing it to
`addPotential` plugs it in.

```console
$ clang++ -std=c++17 plug.cpp -o plug
$ ./plug
LJ only:          U(0.95) = 1.96097, U(1.5) = -0.320337
  plug: Lennard-Jones
  plug: soft repulsion
LJ + soft:        U(0.95) = 1.98597, U(1.5) = -0.320337
```

At 0.95σ the soft repulsion adds $10 \times (1 - 0.95)^2 = 0.025$ to the LJ
energy; at 1.5σ it is beyond its range of 1.0 and adds nothing. The same
call, `calculator.energy(r)`, gave both results; only the list of plugs
changed.

## How the right formula is found

Line 68 is the same line for both plugs, yet it runs the LJ formula for one
and the soft-repulsion formula for the other. The compiler arranges this
with one hidden member ({numref}`cpp5-dispatch-figure`). For every class
with virtual functions it writes a table of that class's versions of them.
Every object of such a class holds, as a hidden first member, a pointer to
its class's table. `p->energy(r)` follows the object's table pointer and
runs the `energy` it finds there. This is **virtual dispatch**.

```{figure} ../figures/cpp5_dispatch.svg
:name: cpp5-dispatch-figure

The force step's list holds pointers to objects of different classes. Each
object carries a hidden pointer to its class's table of functions, and
`p->energy(r)` runs the version in that table.
```

The hidden member is visible in the object's size. This program compares an
LJ struct with the same three `double`s and no virtual functions against the
`LJPotential` plug:

```console
$ ./sizes
PlainLJ (no virtual functions): 24 bytes
LJPotential (plugs into Potential): 32 bytes
```

Three `double`s are 24 bytes; the plug carries 8 more, the table pointer.
(The C++ standard does not say how virtual dispatch must work; this table
scheme is what clang, GCC and Microsoft's compiler all use.)

:::{admonition} If you know Python
:class: tip
Python finds every method this way: `p.energy(r)` looks up `energy` on the
object's class when the line runs, and any object with an `energy` method
works. C++ looks up at run time only functions marked `virtual`, and only
classes that inherit from `Potential` fit a `shared_ptr<Potential>`; the
compiler checks both before the program runs.
:::

## Why the list holds pointers

The list holds pointers, not the plugs themselves. Holding a `Potential`
directly is not possible, because `Potential` is abstract:

```cpp
    LJPotential lj(1.0, 1.0, 2.5);
    Potential copy = lj;
```

```console
$ clang++ -std=c++17 by_value.cpp -o by_value
by_value.cpp:61:15: error: variable type 'Potential' is an abstract class
   61 |     Potential copy = lj;
      |               ^
by_value.cpp:11:20: note: unimplemented pure virtual method 'energy' in 'Potential'
   11 |     virtual double energy(double r) const = 0;
      |                    ^
by_value.cpp:12:25: note: unimplemented pure virtual method 'name' in 'Potential'
   12 |     virtual std::string name() const = 0;
      |                         ^
1 error generated.
```

A variable of type `Potential` would have room only for a `Potential`'s
members, not for an `LJPotential`'s ε, σ and cutoff, nor a
`SoftRepulsion`'s. A pointer is the same size whatever it points to, so a
vector of pointers can hold plugs of every kind side by side.

## override catches mistakes

`override` makes the compiler check that a function really plugs into the
socket. In this version, plug 2's `energy` lost its `const`:

```cpp
    double energy(double r) override {
```

```console
$ clang++ -std=c++17 override_typo.cpp -o override_typo
override_typo.cpp:45:29: error: non-virtual member function marked 'override' hides virtual member function
   45 |     double energy(double r) override {
      |                             ^
override_typo.cpp:11:20: note: hidden overloaded virtual function 'Potential::energy' declared here: different qualifiers ('const' vs unqualified)
   11 |     virtual double energy(double r) const = 0;
      |                    ^
1 error generated.
```

Without `override`, the same mistake still fails, but far from the cause:
this `energy` becomes a separate function that does not fill the socket,
`SoftRepulsion` stays abstract, and the error appears inside the standard
library's `shared_ptr` code, where `make_shared` tries to make the object:

```console
.../c++/v1/__memory/compressed_pair.h:66:7: error: field type 'SoftRepulsion' is an abstract class
```

With `override`, the error is on the line that is wrong, and the note names
the line in the socket it fails to match.

## Where the engine uses it

The engine's socket is `Potential`, declared next to `ForceCalculator`. Its
pure virtual functions are the two questions of the architecture page:

- `calculateEnergy(const Geometry& geometry)`, the total energy of all atoms;
- `calculateForces(const Geometry& geometry, std::vector<std::array<double, 3>>& forces)`,
  the force on every atom;

plus `getName()` and a virtual destructor. `LJPotential` and `DFTPotential`
both inherit from it with `: public Potential` and mark their versions
`override`. `ForceCalculator` keeps its plugs in a
`std::vector<std::shared_ptr<Potential>>`, adds them with `addPotential`,
and adds up their energies and forces in a loop, as here. The engine's
functions take a whole `Geometry` rather than one distance, because one
call computes every pair.

## Check yourself

:::{dropdown} 1. What would make LJPotential abstract again?
Removing (or misspelling, without `override`) its version of any pure
virtual function of `Potential`, for example `name()`. Then `make_shared<LJPotential>`
would fail to compile, as `Potential copy = lj;` did.
:::

:::{dropdown} 2. You write a third plug, HarmonicBond, inheriting from Potential. Which lines of ForceCalculator change?
None. `ForceCalculator` only calls `energy` and `name` through
`Potential` pointers; plugging in the new class takes one `addPotential`
call in `main`.
:::

:::{dropdown} 3. Why is LJPotential 32 bytes when its three members take 24?
Because it has virtual functions: every object holds a hidden 8-byte
pointer to its class's table of functions, which `p->energy(r)` follows.
:::

:::{dropdown} 4. What does calculator.energy(0.5) return once both plugs are in?
The sum of both plugs at r = 0.5: the LJ energy, $4[(1/0.5)^{12} - (1/0.5)^{6}] = 16128$,
plus the soft repulsion, $10 \times (1 - 0.5)^2 = 2.5$, so 16130.5. At half
of σ the LJ wall dwarfs everything else.
:::

## Next

`plug.cpp` is one file of 93 lines, and the engine is many thousands. The
next chapter splits a program across several files, the way the engine is
organised: declarations in headers, code in source files, joined by the
linker.
