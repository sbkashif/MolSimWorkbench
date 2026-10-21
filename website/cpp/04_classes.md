# Classes

So far `lj_energy` takes ε and σ as arguments on every call, and nothing
stops a call from passing σ where ε belongs (quiz 2 of
[Functions](02_functions.md)). An LJ energy function is really one thing
with three settings, ε, σ and the cutoff, and one job, giving the energy at
a distance. A **class** bundles the settings and the functions that use them
into a new type. The engine is built from classes; this chapter writes a
small `LJPotential` of its own.

## A class bundles values and functions

```{literalinclude} examples/ch04/lj_class.cpp
:language: cpp
:linenos:
:caption: lj_class.cpp
```

Lines 4–24 define the class; {numref}`cpp4-objects-figure` shows its parts
and two objects made from it.

- **Data members,** lines 21–23: `epsilon_`, `sigma_` and `cutoff_` are
  variables that belong to the class. The trailing `_` is only a naming
  habit, to tell members apart from other variables at a glance.
- **Constructor,** lines 6–10: a function with the same name as the class
  and no return type. It runs once, when an object is made, and here copies
  its three arguments into the data members.
- **Member function,** lines 12–18: `energy` is the formula from
  [Functions](02_functions.md), with two changes. It reads ε and σ from the
  data members instead of taking them as arguments, and it returns 0 beyond
  the cutoff, as the simulation does.
- **`public:` and `private:`,** lines 5 and 20: everything after `public:`
  can be used from outside the class; everything after `private:` can be
  used only by the class's own functions.

```{figure} ../figures/cpp4_objects.svg
:name: cpp4-objects-figure

The class is a blueprint, written once. Each object made from it holds its
own copy of the data members; the member functions are shared.
```

`main` makes two **objects** of the class, each with its own settings.
`LJPotential argon(1.0, 1.0, 2.5);` makes an object named `argon` and runs
the constructor with those three arguments. `argon.energy(1.5)` calls
`energy` on `argon`, so inside the function `epsilon_` means `argon`'s
`epsilon_`:

```console
$ ./lj_class
argon:  U(1.5) = -0.320337
sticky: U(1.5) = -0.640673
argon:  U(3.0) = 0
```

The same values as in chapter 2: doubling ε doubles the energy. The last
line is 0 because 3.0 is beyond `argon`'s cutoff of 2.5.

:::{admonition} If you know Python
:class: tip
This is Python's `class LJPotential:` with `__init__` as the constructor and
`self.epsilon` as a member. In C++ the members are declared in the class
with their types, and `self` is not written: inside a member function,
`epsilon_` already means "this object's `epsilon_`".
:::

## Private members are protected by the compiler

Because the data members are private, code outside the class cannot change
them. This line in `main` tries:

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

The word `const` after `energy(double r)` on line 12 is a promise: this
function only reads the object's members and never changes them. The
compiler holds the function to it. This version tries to change `cutoff_`
inside `energy`:

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

The positions of chapter 3 belong together too. A **struct** is a class
whose members are public unless marked otherwise; it suits a plain bundle of
data:

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
chapter 3's three atoms; the fourth atom is more than 2.5σ from all of them,
so the cutoff makes its pairs contribute 0.

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

:::{dropdown} 1. After LJPotential krypton(1.5, 1.1, 2.75); what does krypton.energy(1.5) compute, and which ε does it use?
The LJ energy at r = 1.5 with ε = 1.5, σ = 1.1 and cutoff 2.75: the values
the constructor stored in `krypton`'s own members. `argon` and `sticky` are
not affected.
:::

:::{dropdown} 2. Why does argon.epsilon_ = 2.0; fail to compile, while argon.energy(1.5) works?
`epsilon_` is declared after `private:`, so only the class's own functions
may use it. `energy` is declared after `public:`, so any code may call it.
:::

:::{dropdown} 3. total_energy takes const Geometry& geometry. What would change if it took Geometry geometry instead?
The function would get a copy of the whole geometry, including every
position, on each call; the printed address inside would differ from the one
in `main`. The result would be the same, only slower for many atoms.
:::

## Next

`LJPotential` is one energy function. The architecture page described the
energy function as a plug: LJ, DFT or any other, in the same place. The next
chapter writes the socket, a class `Potential` that every plug fits.
