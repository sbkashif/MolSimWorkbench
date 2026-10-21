# Classes

Chapters 2 and 3 gave two tools: functions, and vectors to hold many
values. This chapter first takes them as far as they go, computing the
energy of chapter 3's atoms for two kinds of atom, and finds where they stop
helping. Then it introduces the tool the engine is built from, the
**class**, and writes a small `LJPotential` of its own.

## How far functions and vectors go

An LJ energy function has three settings: ε, σ and the cutoff. With only
functions, each setting is a separate variable, and every function that
needs them takes all three as arguments. `loose_settings.cpp` computes the
total energy of chapter 3's three atoms for two kinds of atom, argon and a
stickier one with twice the ε:

```{literalinclude} examples/ch04/loose_settings.cpp
:language: cpp
:linenos:
:caption: loose_settings.cpp
```

`lj_energy` (lines 6–12) is the function of chapter 2 with the cutoff
added, and `total_energy` (lines 21–31) is the double loop of chapter 3. The
first two calls in `main` are correct. The last two each contain a slip of
the kind that is easy to make when a call lists six or seven values:

```console
$ clang++ -std=c++17 -Wall -Wextra loose_settings.cpp -o loose_settings
$ ./loose_settings
argon:  U = -1.28933
sticky: U = -2.57866
argon, sigma and cutoff swapped: U = 0
argon, with sticky's epsilon:    U = -2.57866
```

The compiler accepted both slips without a single warning, and both give a
wrong number:

- **Swapped arguments** (line 53) pass σ = 2.5 and a cutoff of 1.0. Every
  pair is farther apart than 1.0, so the energy comes out exactly 0, which
  looks like a plausible answer. The compiler checks only that each argument
  is a `double`, and all of them are.
- **Mixed systems** (line 55) take ε from the sticky atoms and σ and the
  cutoff from argon. The result is the sticky energy, printed under argon's
  name. Nothing in the program says that `argon_sigma` belongs with
  `argon_epsilon`; only the names do, and the compiler does not read names.

The trouble grows with the program. The simulation also needs forces, so a
`total_forces` function would take the same three settings, and so would
every function after it. Adding a fourth setting, such as a shift of the
energy at the cutoff, would mean changing every one of those functions and
every call to them.

## From packaging code to packaging values

Each chapter so far has packaged something under one name. A function
packages lines of code: the formula is written once and run for any
distance ([Functions](02_functions.md)). A vector packages many values of
one type: three positions, or 512, travel as one variable
([Many values: vectors and loops](03_vectors_and_loops.md)).

`loose_settings.cpp` needs a third kind of package: a few values with
different meanings, ε, σ and the cutoff, kept together as one argon
potential, along with the functions that use them.

C++ lets you define such a package as a new **type**.
[A first program](01_first_program.md) met types: `double` is a type, and
`double r = 1.5;` makes a variable of that type. A **class** is a type you
write yourself. Once a class `LJPotential` is defined,

```cpp
LJPotential argon(1.0, 1.0, 2.5);
```

makes a variable of that type in the same way, with ε = 1.0, σ = 1.0 and a
cutoff of 2.5 as its starting values. A variable whose type is a class is
called an **object**. One class can make many objects, just as one type
`double` can make many variables: `argon` and `sticky` below are two
objects of the same class, each holding its own ε, σ and cutoff.

## Writing the class

A class has to say three things: what each object holds, how an object gets
its starting values, and what an object can do. `lj_class.cpp` writes all
three for the LJ potential, then makes the two objects:

```{literalinclude} examples/ch04/lj_class.cpp
:language: cpp
:linenos:
:caption: lj_class.cpp
```

Lines 4–24 are the class. It starts with the word `class` and the new
type's name, and everything up to the closing `};` belongs to it. The rest
of this section reads it in the order of the three questions;
{numref}`cpp4-objects-figure` shows the result.

### What each object holds: data members

Lines 21–23 declare three variables exactly as
[A first program](01_first_program.md) did: a type, then a name. Written
inside a class, they are **data members**: every object made from the class
gets its own set of them. `argon` has an `epsilon_`, `sigma_` and `cutoff_`;
`sticky` has another three. These replace the six loose variables of
`loose_settings.cpp`. The trailing `_` is only a naming habit, to tell
members apart from other variables at a glance.

### How an object gets its values: the constructor

A `double` gets its value when it is made: `double r = 1.5;`. An object has
three values to fill in, so the class says how. Lines 6–10 are a
**constructor**. It looks like a function from [Functions](02_functions.md),
with parameters in parentheses and a body in braces, and differs in two
ways:

- Its name is the class's name, `LJPotential`.
- It has no return type. It does not hand a value back; its job is to fill
  in the new object's data members, here by copying its three arguments
  into them.

You never call a constructor by name. It runs by itself, once, when an
object is made: `LJPotential argon(1.0, 1.0, 2.5);` makes `argon` and runs
the constructor with 1.0, 1.0 and 2.5 as its arguments, matched to
`epsilon`, `sigma` and `cutoff` by position, as for any function call.

### What an object can do: member functions

Lines 12–18 are `lj_energy` from `loose_settings.cpp`, moved inside the
class and renamed `energy`. A function written inside a class is a **member
function**. Its parameter list has shrunk to `double r`: ε, σ and the cutoff
are no longer passed in, because the function reads them from the data
members. (The word `const` after the parentheses is explained below.)

A member function is called on an object, with a dot:

```cpp
argon.energy(1.5)
```

The dot says whose data members to use. Inside the function, `epsilon_`
means `argon`'s `epsilon_` for this call, and `sticky`'s for
`sticky.energy(1.5)`.

### Who may use what: public and private

One more line in the class decides which parts outside code may touch.
Everything after `public:` (line 5) can be used anywhere: `main` may make
objects and call `energy`. Everything after `private:` (line 20) can be used
only by the class's own functions. The data members are private, for a
reason the next section shows.

```{figure} ../figures/cpp4_objects.svg
:name: cpp4-objects-figure

The class is a blueprint, written once. Each object made from it holds its
own copy of the data members; the member functions are shared.
```

### Running it

`main` (lines 26–33) makes `argon` and `sticky` and asks each for the
energy at r = 1.5, then asks `argon` again at 3.0:

```console
$ ./lj_class
argon:  U(1.5) = -0.320337
sticky: U(1.5) = -0.640673
argon:  U(3.0) = 0
```

The same values as in chapter 2: doubling ε doubles the energy. The last
line is 0 because 3.0 is beyond `argon`'s cutoff of 2.5.

Compare the calls with those of `loose_settings.cpp`. `argon.energy(1.5)`
names one object and one distance; argon's ε, σ and cutoff come with the
object. There is no order of settings to get wrong, and no way to take ε
from one kind of atom and σ from another. The settings are written out once,
when the object is made.

:::{admonition} If you know Python
:class: tip
This is Python's `class LJPotential:` with `__init__` as the constructor and
`self.epsilon` as a member. In C++ the members are declared in the class
with their types, and `self` is not written: inside a member function,
`epsilon_` already means "this object's `epsilon_`".
:::

## Private members are protected by the compiler

In `loose_settings.cpp`, any line of the program could change
`argon_epsilon`, on purpose or by a slip, and every later call would quietly
use the new value. Making the data members private closes that door: code
outside the class cannot change them. This line in `main` tries:

```cpp
    argon.epsilon_ = 2.0;
```

```console
$ clang++ -std=c++17 private_access.cpp -o private_access
private_access.cpp:28:11: error: 'epsilon_' is a private member of 'LJPotential'
   28 |     argon.epsilon_ = 2.0;
      |           ^
private_access.cpp:21:12: note: declared private here
   21 |     double epsilon_;
      |            ^
1 error generated.
```

The only way to set ε is through the constructor, so every `LJPotential`
gets its settings in one checked place. The engine's `LJPotential` keeps its
parameters private in the same way and offers public functions such as
`setCutoff` to change them.

## const member functions promise not to change the object

Private members stop outside code from changing an object. The class's own
functions can still change it, and most of them should not: asking for an
energy is a question, and a question should leave the potential as it was.

C++ marks "does not change" with the word `const`. Written after the
parameter list of a member function, as on line 12, `double energy(double r)
const`, it is a promise: this function only reads the object's members and
never changes them. The compiler holds the function to it. This version
tries to change `cutoff_` inside `energy`:

```cpp
        if (r > cutoff_) {
            cutoff_ = r;
            return 0.0;
        }
```

```console
$ clang++ -std=c++17 const_member.cpp -o const_member
const_member.cpp:14:21: error: cannot assign to non-static data member within const member function 'energy'
   14 |             cutoff_ = r;
      |             ~~~~~~~ ^
const_member.cpp:12:12: note: member function 'LJPotential::energy' is declared const here
   12 |     double energy(double r) const {
      |     ~~~~~~~^~~~~~~~~~~~~~~~~~~~~~
1 error generated.
```

Asking for an energy should never change the potential, so `energy` is
`const`. The engine marks its read-only functions the same way, for example
`getName() const` in `LJPotential` and `getNumberOfAtoms() const` in
`Geometry`.

## A geometry, passed without copying

The settings are now one object, but the positions are still a loose
vector, handed to every function that needs them. They can be packaged the
same way: a type `Geometry` whose object holds the positions of all atoms
(the engine's `Geometry` also holds the box).

The positions need no protection and no functions of their own here; the
package is plain data. For that case C++ has a shorter form, the
**struct**. A struct is a class whose members are public unless marked
otherwise, so it needs no `public:` line:

```{literalinclude} examples/ch04/geometry.cpp
:language: cpp
:lines: 6-8
:lineno-start: 6
:caption: geometry.cpp (the struct)
```

`LJPotential` gains one member function that adds up the energy of every
pair, with the double loop of chapter 3 (the class is otherwise as above):

```{literalinclude} examples/ch04/geometry.cpp
:language: cpp
:lines: 26-39
:lineno-start: 26
:caption: geometry.cpp (new member function)
```

The parameter is `const Geometry& geometry`. The `&` makes it a
**reference**: a second name for the caller's object, not a copy of it.
[Functions](02_functions.md) showed that an ordinary parameter is a copy,
and promised a way to avoid copying a list of many atoms; this is it. The
`const` in front promises that the function will not change the geometry
through this name. Line 28 uses a reference the same way, to give
`geometry.positions` the short name `p` without copying it.

`main` builds a geometry of four atoms (the fourth far from the others),
prints its address, and asks for the total energy:

```{literalinclude} examples/ch04/geometry.cpp
:language: cpp
:lines: 47-57
:lineno-start: 47
:caption: geometry.cpp (main)
```

```console
$ ./geometry
in main: geometry is at 0x16f1e2880
  inside: geometry is at 0x16f1e2880
total U = -1.28933
```

The two addresses are the same: inside `total_energy`, `geometry` is
`main`'s object itself ({numref}`cpp4-reference-figure`). The total matches
chapter 3's three atoms, and argon in `loose_settings.cpp`; the fourth atom
is more than 2.5σ from all of them, so the cutoff makes its pairs
contribute 0.

The call is now `argon.total_energy(geometry)`: two objects, one holding the
settings and one holding the atoms, where `loose_settings.cpp` needed a
vector and three loose numbers in the right order. A function for the forces
would take the same two objects, and a fourth setting would be one more data
member, with no call to change.

```{figure} ../figures/cpp4_reference.svg
:name: cpp4-reference-figure
:width: 620px

A `const Geometry&` parameter names the caller's object. Compare the copy
made for an ordinary parameter in chapter 2.
```

### Where the engine uses it

The engine has a class `Geometry` and a class `LJPotential`, in the same
roles:

- `Geometry` keeps the atom positions as a private
  `std::vector<std::array<double, 3>>`, with public functions to read them
  (`getPositions`, `getCoordinates`) and to change one (`setCoordinates`).
- `LJPotential` keeps ε, σ and the cutoff private and computes energies and
  forces with `calculateEnergy(const Geometry& geometry)` and
  `calculateForces`, which take the geometry by `const` reference, as
  `total_energy` does here.

The engine's `LJPotential` also starts with `class LJPotential : public
Potential`. That `: public Potential` is the plug from
[A minimal molecular simulation algorithm](../molsim/architecture.md#the-energy-function-is-a-plug),
and the subject of the next chapter.

## Check yourself

:::{dropdown} 1. In loose_settings.cpp, why does the compiler accept total_energy(positions, argon_epsilon, argon_cutoff, argon_sigma)?
It checks only the types of the arguments, and all three settings are
`double`s. It cannot know that the third argument was meant to be σ; the
names of the variables play no part in matching arguments to parameters.
:::

:::{dropdown} 2. After LJPotential krypton(1.5, 1.1, 2.75); what does krypton.energy(1.5) compute, and which ε does it use?
The LJ energy at r = 1.5 with ε = 1.5, σ = 1.1 and cutoff 2.75: the values
the constructor stored in `krypton`'s own members. `argon` and `sticky` are
not affected.
:::

:::{dropdown} 3. Why does argon.epsilon_ = 2.0; fail to compile, while argon.energy(1.5) works?
`epsilon_` is declared after `private:`, so only the class's own functions
may use it. `energy` is declared after `public:`, so any code may call it.
:::

:::{dropdown} 4. total_energy takes const Geometry& geometry. What would change if it took Geometry geometry instead?
The function would get a copy of the whole geometry, including every
position, on each call; the printed address inside would differ from the one
in `main`. The result would be the same, only slower for many atoms.
:::

## Next

`LJPotential` is one energy function. The architecture page described the
energy function as a plug: LJ, DFT or any other, in the same place. The next
chapter writes the socket, a class `Potential` that every plug fits.
