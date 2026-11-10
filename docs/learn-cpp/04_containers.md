# Chapter 4 — Containers: from two atoms to N

Plain version: [04p_containers_plain.md](04p_containers_plain.md).
Code: [`examples/04/`](examples/04/) — `mini_lj.cpp` plus two toy programs. Every output below was
produced by compiling and running them with Apple clang 17 on an M1 Mac.

**After this chapter you can say:**

- what `std::vector` is: a growable sequence of one element type, which `b = a` copies whole;
- why `v[i]` does no bounds check, and what `.at(i)` does instead;
- what `std::map` is, and why `m[key]` on a missing key **inserts** a default entry instead of failing;
- what a `struct` is: a new type made of named fields, each with its own fixed type;
- how the `i < j` double loop visits every pair exactly once, and why its indices are `std::size_t`;
- that mini_lj, with these pieces, reproduces the repo's reference energy for 4096 atoms to every printed
  digit.

## Where we are

At the end of chapter 3, mini_lj had two atoms, `a` and `b`, each a named `Vec3`, and one call to
`pairForce`. That does not scale: a real system has hundreds or thousands of atoms, and they cannot be
thousands of named variables. Each atom also has a **type** ("LJ", "Ar", …) that decides its ε and σ. This
chapter brings in the standard **containers** — `std::vector` for "one entry per atom" and `std::map` for
"parameters per type name" — and a loop over every pair (i, j). At the end, mini_lj computes the same
energy as the repo for 4096 atoms.

Time: about 35 minutes, including running the examples.

```bash
cd docs/learn-cpp/examples/04
mkdir -p out          # compiled programs go here (git-ignored)
```

---

## 4.0 The standard containers

A **container** is an object whose job is to hold other objects. C++'s standard library provides them as
templates — you fill in the element type in angle brackets, as with `std::array<double, 3>` in chapter 3.

| C++ | Header | What it holds | Size | Element type |
|---|---|---|---|---|
| `std::vector<T>` | `<vector>` | a sequence, indexed from 0 | grows and shrinks | one type `T`, fixed |
| `std::array<T, N>` | `<array>` | exactly `N` elements, indexed from 0 | fixed at compile time | one type `T` |
| `std::map<K, V>` | `<map>` | key → value pairs, looked up by key | grows and shrinks | keys all `K`, values all `V` |
| `std::string` | `<string>` | text | grows and shrinks | characters |
| `struct` | — | a fixed set of named fields | fixed | each field has its own type |

Two rules from earlier chapters apply to all of them:

- **one element type, fixed** (chapter 2). A `std::vector<double>` holds `double`s and nothing else;
- **`b = a` copies everything inside** (chapter 3). A `std::vector` of 4096 positions copied is 4096
  positions duplicated. That is why the repo hands containers to functions as `const std::vector<…>&`.

> **Unlike Python:** a `list` or `dict` holds references to objects that live elsewhere, of any mix of
> types. A C++ container holds the values themselves, all of one type, packed in its own memory — which is
> why copying the container copies every value.

---

## 4.1 `std::vector`: a list of one type

`examples/04/vector_demo.cpp`, first part:

```cpp
std::vector<double> xs;                 // empty, like xs = []
xs.push_back(1.0);                      // xs.append(1.0)
xs.push_back(2.0);
std::cout << "size = " << xs.size() << ", xs[1] = " << xs[1] << std::endl;

std::vector<double> zeros(4, 0.0);      // round brackets: (how many, of what) -> [0.0] * 4
std::vector<double> listed = {4, 0.0};  // braces: the elements themselves -> [4.0, 0.0]
std::cout << "zeros.size() = " << zeros.size() << ", listed.size() = " << listed.size() << std::endl;
std::cout << "sizeof(xs) = " << sizeof(xs) << ", sizeof(zeros) = " << sizeof(zeros) << std::endl;

// Assignment copies every element.
std::vector<double> ys = xs;
ys[0] = 99.0;
std::cout << "after ys[0] = 99: xs[0] = " << xs[0] << ", ys[0] = " << ys[0] << std::endl;

// A vector of Vec3: one block of numbers, x y z x y z ...
std::vector<Vec3> positions = {{0.0, 0.0, 0.0}, {1.2, 0.0, 0.0}};
std::cout << "positions[1][0] = " << positions[1][0] << std::endl;
```
```
$ c++ -std=c++17 -Wall -O2 vector_demo.cpp -o out/vector_demo && ./out/vector_demo
size = 2, xs[1] = 2
zeros.size() = 4, listed.size() = 2
sizeof(xs) = 24, sizeof(zeros) = 24
after ys[0] = 99: xs[0] = 1, ys[0] = 99
positions[1][0] = 1.2
...
```

| C++ `std::vector<double>` | What it does |
|---|---|
| `std::vector<double> xs;` | an empty vector |
| `xs.push_back(1.0);` | append one element at the end |
| `xs.size()` | number of elements — returns a `std::size_t` (chapter 2) |
| `xs[1]` | element 1 (counting from 0) |
| `std::vector<double> zeros(4, 0.0);` | four elements, each `0.0` — *(count, value)* in round brackets |
| `std::vector<Vec3> positions = {{0.0, 0.0, 0.0}, {1.2, 0.0, 0.0}};` | two `Vec3`s, listed in braces |
| `ys = xs` | copies every element (`after ys[0] = 99: xs[0] = 1`) |
| `xs.resize(n)` | grow or shrink to n elements |

`.push_back`, `.size()` and `.resize()` are **member functions**: functions that belong to the object and
are called with a dot. Chapter 6 shows how a class defines them.

`std::vector<Vec3>` — a vector of chapter 3's trays of three — is how mini_lj and the repo store positions.
`positions[1]` is atom 1's `Vec3`, and `positions[1][0]` is its x coordinate.

> **Unlike Python:** `ys = xs` on a list gives two names for one list, so `ys[0] = 99` would change `xs`
> too. Here the output shows `xs[0] = 1` untouched: `ys` is a separate vector with its own copy of every
> element.

### Round brackets or braces: they mean different things

`std::vector<double> zeros(4, 0.0);` makes **four** elements, each `0.0`. `std::vector<double> listed =
{4, 0.0};` makes **two** elements, `4.0` and `0.0` — the output above shows `zeros.size() = 4,
listed.size() = 2`. Braces list the elements; round brackets pass "how many, of what". This is the one
place in the course where the two spellings disagree.

---

## 4.2 Indexing: `[]` does not check, `.at()` does

`xs[5]` on a two-element vector does **not check**: it computes where element 5 would be and reads that
memory. The result is undefined behaviour (chapter 2) — usually a nonsense number, sometimes a crash, never
a clear error. `.at(i)` is the checked version; it throws an exception. From `vector_demo.cpp`:

```cpp
try {
    std::cout << xs.at(5) << std::endl;
} catch (const std::out_of_range& e) {
    std::cout << "xs.at(5) threw std::out_of_range" << std::endl;
}
```
```
xs.at(5) threw std::out_of_range
```

(`try { … } catch (const T& e) { … }` runs the `try` block and, if it throws a `T`, jumps to the `catch`
block — and the `const T&` is chapter 3's "read-only, not copied".)

| | `xs[5]` | `xs.at(5)` |
|---|---|---|
| out of range | undefined behaviour, no message | throws `std::out_of_range` |
| cost | no check | a check every time |

> **Unlike Python:** every `xs[5]` on a list is checked and raises `IndexError`. C++ makes the check
> opt-in: plain `[]` trusts you, so an off-by-one reads some other memory and carries on.

The repo's hot loops use `[]`. The loop bounds (`i < positions.size()`) already guarantee the index is in
range, so a second check on every access would be wasted work. That is a decision you make on purpose.

### `size()` is unsigned (skip if you never subtract from a size)

Chapter 2 warned that `std::size_t` wraps around below zero. A size is a `std::size_t`, so:

```cpp
std::vector<double> empty;
std::cout << "empty.size() - 1 = " << empty.size() - 1 << std::endl;
```
```
empty.size() - 1 = 18446744073709551615
```

A loop `for (std::size_t i = 0; i < v.size() - 1; ++i)` on an empty vector therefore runs about 18
quintillion times instead of zero. mini_lj's pair count, `positions.size() * (positions.size() - 1) / 2`,
is safe because there is always at least one atom; the pair loop itself avoids `- 1` and writes
`j = i + 1` instead.

---

## 4.3 `std::array` or `std::vector`?

Both hold elements of one type side by side in memory. The difference is when the size is decided.

| | `std::array<double, 3>` | `std::vector<double>` |
|---|---|---|
| size | fixed, part of the type, known when compiling | chosen while running; can change |
| where the numbers live | inside the variable itself (24 bytes for three doubles) | in a separate block of memory the vector manages |
| `sizeof` | 24 — the three numbers | 24 — but that is the vector's own bookkeeping (where its block is, how full, how big), not the elements: `sizeof(xs)` and `sizeof(zeros)` above are both 24, for 2 and 4 elements |
| used for | one position, one force, the box (always 3) | one entry per atom (N decided at run time) |

So the repo's type for "all positions" is the two nested together: `std::vector<std::array<double, 3>>`
— a growable list of fixed trays. Because each tray's three numbers sit inside it, and the vector stores
its trays side by side, all positions end up in one block: x₀ y₀ z₀ x₁ y₁ z₁ x₂ … That layout has a name,
**Array of Structures (AoS)**, and it matters a great deal for speed. Its alternative, three separate
vectors of x, y and z, is **Structure of Arrays (SoA)**. Which one is faster for the LJ loop, and why, is
measured in [learn-parallel lesson 02](../learn-parallel/02_serial_refactor.md); the glossary entry
[Memory and layout](../learn-parallel/GLOSSARY.md#memory-and-layout) has the one-paragraph version. For
now, just be able to say: `vector<Vec3>` is AoS.

---

## 4.4 `std::string`: text that owns its letters

`std::string` (header `<string>`) is C++'s text type. It is **mutable** — `s[0] = 'X';` changes it in
place — and `t = s` **copies** it. Chapter 3's `argv_pointer.cpp` did both. Text in double quotes, like `"LJ"`, is the older C form (a `const char[3]`,
chapter 2) and converts to a `std::string` automatically wherever one is expected.

mini_lj uses it for atom type names: `std::vector<std::string> types;` — one name per atom — and, in the
repo, `Geometry` stores exactly that (§4.10).

---

## 4.5 `struct`: named fields with fixed types

mini_lj needs to keep ε and σ together for each atom type. `examples/04/mini_lj.cpp:13-16`:

```cpp
13  struct Params {
14      double epsilon;
15      double sigma;
16  };
```

This declares a new type, `Params`, with two named fields. The field types are enforced by the compiler,
the set of fields is fixed, and the fields sit side by side in memory: a `Params` is exactly two `double`s,
16 bytes.

| C++ `struct` | What it does |
|---|---|
| `Params p = {1.0, 1.0};` | create one, fields filled in declaration order |
| `p.epsilon` | read or write one field |
| `q = p` | copies both fields into a separate `Params` |

> **Unlike Python:** a `@dataclass` is an ordinary object you can bolt new attributes onto
> (`p.extra = 1`), its type hints are never checked, and `q = p` shares it. A `struct` is a fixed memory
> layout: no field can be added later, a wrong type does not compile, and `q = p` copies the bytes.

Don't forget the `;` after the closing `}` — a `struct` declaration is a statement. Chapter 6 shows that
`struct` and `class` are nearly the same thing, and adds functions to them. The repo's own version of
`Params` is `LJPotential::AtomParameters`, `src/engine/potentials/lj_potential.h:65-68` — the same two
fields.

---

## 4.6 `std::map`: a lookup table — with one trap

`std::map<K, V>` (header `<map>`) maps keys of type `K` to values of type `V`. mini_lj maps a type name to
its parameters: `std::map<std::string, Params> params;`.

The trap is reading with `[]`. `m["Ar"]` for a missing key **creates** an entry `"Ar"` with a default
value (zeros, for a struct of numbers) and gives you that. It never fails. `examples/04/map_demo.cpp`:

```cpp
std::map<std::string, Params> params;
params["LJ"] = {1.0, 1.0};
std::cout << "size = " << params.size() << std::endl;

// Reading a missing key with [] does not fail. It INSERTS a default entry.
const Params& ar = params["Ar"];
std::cout << "params[\"Ar\"].epsilon = " << ar.epsilon << ", size now = " << params.size() << std::endl;

// .at() refuses: it throws std::out_of_range.
try {
    params.at("Ne");
} catch (const std::out_of_range& e) {
    std::cout << "params.at(\"Ne\") threw std::out_of_range" << std::endl;
}

// .find() asks without inserting. end() means "not found".
if (params.find("Xe") == params.end()) std::cout << "Xe not found, size still " << params.size() << std::endl;

// Iteration visits keys in sorted order.
for (const auto& entry : params) std::cout << entry.first << " ";
std::cout << std::endl;
```
```
$ c++ -std=c++17 -Wall -O2 map_demo.cpp -o out/map_demo && ./out/map_demo
size = 1
params["Ar"].epsilon = 0, size now = 2
params.at("Ne") threw std::out_of_range
Xe not found, size still 2
Ar LJ
```

Line two of the output is the trap in action: we only *read* `params["Ar"]`, and the map grew from 1 to
2 entries, with an "Ar" whose ε is 0. A typo in a type name does not fail loudly; it silently becomes an
atom with no interactions.

> **Unlike Python:** `d["Ar"]` on a missing key raises `KeyError`, so reading can never change a `dict`.
> In C++ the same-looking read is a write: `m[k]` inserts. The C++ spelling of "read, and fail if it is
> missing" is `m.at(k)`.

| You want… | C++ `std::map` |
|---|---|
| set a value | `m[k] = v;` |
| read, key must exist | `m.at(k)` → throws `std::out_of_range` if missing |
| read, missing key is a bug you want hidden | `m[k]` → **inserts** a default and returns it |
| ask whether it is there | `m.find(k) != m.end()` (C++20 adds `m.contains(k)`; this repo is C++17) |
| number of entries | `m.size()` |
| loop over entries | `for (const auto& entry : m)` — `entry.first` is the key, `entry.second` the value |
| order of iteration | **sorted by key** (`Ar` before `LJ`), not insertion order |

`.find(k)` returns an **iterator** — a position inside the container — and `.end()` is the special
position "one past the last entry", used to mean "not found". You do not need iterators beyond this one
comparison for now.

`std::map` keeps its keys sorted by storing them in a tree. Looking up a key walks down the tree,
comparing strings as it goes — far slower than indexing a vector. The glossary's
[Red-black tree](../learn-parallel/GLOSSARY.md#memory-and-layout) entry describes this; learn-parallel
lesson 02 measures what it costs in the pair loop.

---

## 4.7 Loops: counting, every pair, and range-for

Chapter 3 introduced `for (int k = 0; k < 3; ++k)`. mini_lj needs three more loop ideas.

**Every pair once.** For N atoms, the pairs are (0,1), (0,2), …, (0,N−1), (1,2), …, (N−2,N−1). The
standard shape:

```cpp
for (std::size_t i = 0; i < positions.size(); ++i) {
    for (std::size_t j = i + 1; j < positions.size(); ++j) {
        // pair (i, j)
    }
}
```

Starting `j` at `i + 1` skips (i, i) — an atom with itself — and skips (j, i) when (i, j) was already
done. That gives N(N−1)/2 pairs: for N = 125, 125 × 124 / 2 = 7750. Picture the N × N grid of all
(i, j): this loop visits only the triangle above the diagonal, which is why the parallel course calls it
the [triangular loop](../learn-parallel/GLOSSARY.md#the-two-ways-to-write-the-loop).

The indices are `std::size_t` because `positions.size()` is. Comparing an `int` index with a size mixes a
signed and an unsigned type, which goes wrong if the `int` is ever negative. Apple clang only warns about it
with `-Wextra` (more warnings than `-Wall`); GCC (the usual compiler on Linux) already warns with `-Wall`.
`examples/04/sign_compare.cpp` has one loop of each kind:

```
$ c++ -std=c++17 -Wall -O2 sign_compare.cpp -o out/sign_compare
$ c++ -std=c++17 -Wall -Wextra -O2 sign_compare.cpp -o out/sign_compare
sign_compare.cpp:8:23: warning: comparison of integers of different signs: 'int' and 'size_type' (aka 'unsigned long') [-Wsign-compare]
    8 |     for (int i = 0; i < v.size(); ++i) sum += v[i];           // int vs std::size_t
      |                     ~ ^ ~~~~~~~~
1 warning generated.
```

The first command prints nothing. The loop with a `std::size_t` index (line 9) never warns.

**`continue`** jumps to the next pass of the innermost loop. mini_lj uses it to skip pairs beyond the
cutoff.

**Range-for**: `for (const auto& entry : params)` visits each element in turn. The `const auto&` is chapter 3: each element is seen through a read-only reference, not copied. Write
`auto&` (no `const`) when the loop body should change the elements, and plain `auto` only when you want a
copy of each.

> **Unlike Python:** `for x in items:` always binds `x` to the element object itself, and rebinding `x`
> does nothing to the list. In C++ you choose: `auto` gives a copy (changes are lost), `auto&` gives the
> element itself (changes stick), `const auto&` gives the element read-only.

**Braces are optional for a one-statement body.** mini_lj's lattice loops (lines 32–33) write
`for (…)` followed directly by the next `for`, without `{`. That is legal: the body is just the next
statement. The repo always writes the braces, and so should you.

---

## 4.8 Building a lattice (and a random number generator)

To have many atoms we need positions. The repo's `LJParticleBoxBuilder::createCubicLattice` puts N atoms on
a cubic grid at a chosen density, nudged slightly at random; mini_lj copies that recipe line for line, so
the two produce **identical positions**. `examples/04/mini_lj.cpp:19-40`:

```cpp
19      const std::size_t numAtoms = (argc >= 2) ? std::stoul(argv[1]) : 125;
20      const double density = 0.8;
21      const double cutoff = 2.5;
22
23      // --- build a cubic lattice, the same recipe as LJParticleBoxBuilder::createCubicLattice
24      const double box = std::cbrt(numAtoms / density);
25      const int perSide = static_cast<int>(std::cbrt(numAtoms) + 0.5);
26      const double spacing = box / perSide;
27      std::mt19937 rng(42);
28      std::uniform_real_distribution<double> jitter(-0.05 * spacing, 0.05 * spacing);
29
30      std::vector<Vec3> positions;          // one entry per atom
31      std::vector<std::string> types;       // one type name per atom
32      for (int i = 0; i < perSide && positions.size() < numAtoms; ++i)
33          for (int j = 0; j < perSide && positions.size() < numAtoms; ++j)
34              for (int k = 0; k < perSide && positions.size() < numAtoms; ++k) {
35                  double x = (i + 0.5) * spacing - box / 2.0 + jitter(rng);
36                  double y = (j + 0.5) * spacing - box / 2.0 + jitter(rng);
37                  double z = (k + 0.5) * spacing - box / 2.0 + jitter(rng);
38                  positions.push_back({x, y, z});
39                  types.push_back("LJ");
40              }
```

| Line | What it does | From chapter |
|---|---|---|
| 19 | N from the command line if given (`std::stoul` = "string to unsigned long"), else 125 | 1 (`argc`/`argv`), 2 (`?:`, `const`) |
| 24 | box length L = (N/ρ)^⅓. `numAtoms / density` is `std::size_t / double`, so `numAtoms` is converted to `double` first — no integer division | 2 |
| 25 | atoms per side: cube root, **+ 0.5, then convert to `int`** — the C++ idiom for "round to nearest whole number" for positive values, because conversion to `int` drops the fraction | 2 (`static_cast`) |
| 32–34 | three nested loops over the grid, each also stopping once N atoms are placed (`&&` is logical "and") | this chapter |
| 35–37 | the grid point, centred in the box, plus a small random nudge | 2 |
| 38 | append a `Vec3` built from braces — chapter 3's brace construction, inside a call | 3 |
| 39 | every atom gets type "LJ" | §4.4 |

### `std::mt19937`: reproducible random numbers (skip if you do not care how the nudge is made)

`std::mt19937 rng(42);` (header `<random>`) creates a random number **generator** — the Mersenne Twister —
seeded with 42. `std::uniform_real_distribution<double> jitter(lo, hi)` turns its raw output into
`double`s spread evenly between `lo` and `hi`. Each call `jitter(rng)` gives the next one. Same seed, same
sequence, every run — which is why mini_lj and the repo, both seeded with 42 and both drawing x, y, z in
the same order, place every atom at exactly the same spot.

---

## 4.9 The pair loop

`examples/04/mini_lj.cpp:42-67`:

```cpp
42      // --- parameters per atom type
43      std::map<std::string, Params> params;
44      params["LJ"] = {1.0, 1.0};
45
46      // --- every pair once: i < j
47      double energy = 0.0;
48      std::size_t inside = 0;
49      for (std::size_t i = 0; i < positions.size(); ++i) {
50          for (std::size_t j = i + 1; j < positions.size(); ++j) {
51              Vec3 d;
52              for (int k = 0; k < 3; ++k) {
53                  d[k] = positions[j][k] - positions[i][k];
54                  d[k] -= box * std::round(d[k] / box);   // minimum image
55              }
56              const double r = std::sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
57              if (r > cutoff) continue;
58              ++inside;
59
60              const Params& pi = params[types[i]];
61              const Params& pj = params[types[j]];
62              const double eps = std::sqrt(pi.epsilon * pj.epsilon);   // geometric mixing
63              const double sig = std::sqrt(pi.sigma * pj.sigma);
64              const double sr6 = std::pow(sig / r, 6);
65              energy += 4.0 * eps * (sr6 * sr6 - sr6);
66          }
67      }
```

| Line | What it does | From chapter |
|---|---|---|
| 43–44 | the type → parameters table, one entry | §4.5, §4.6 |
| 47–48 | accumulators, both given a starting value | 2 |
| 49–50 | every pair once | §4.7 |
| 51 | `Vec3 d;` with no value — safe here only because lines 52–54 set all three elements before any is read | 2 (uninitialised), 3 |
| 53–54 | chapter 3's `displacement` and `applyMinimumImage`, written inline | 3 |
| 56 | the distance, now with `std::sqrt` | 2 |
| 57 | beyond the cutoff (2.5): skip the rest of this pass | §4.7 |
| 60–61 | **`params[types[i]]`**: look up atom i's type name, then its parameters, and hold them by `const` reference (no copy of the `Params`) | 3, §4.6 |
| 62–63 | geometric mixing: ε_ij = √(ε_i ε_j), σ_ij = √(σ_i σ_j) | 2 |
| 64–65 | chapter 2's energy formula, added into the total | 2 |

Lines 60–61 use the trap from §4.6. They are correct here, because every type is "LJ" and "LJ" is in the
map. If an atom had the type "Ar", `params["Ar"]` would quietly insert a zero entry and that atom would
contribute zero energy. The repo guards against exactly this (§4.10).

---

## 4.10 Run it: 125 atoms, then 4096

```bash
c++ -std=c++17 -Wall -O2 mini_lj.cpp -o out/mini_lj
./out/mini_lj
./out/mini_lj 4096
```
```
N = 125, box = 5.38608672508
pairs = 7750, inside cutoff = 3474
E = -611.491665613
N = 4096, box = 17.2354775203
pairs = 8386560, inside cutoff = 113910
E = -20105.493949
```

(`std::cout.precision(12)` on line 70 is chapter 2's 12 significant digits.) Of the 8,386,560 pairs at
N = 4096, only 113,910 — about 1.4% — are inside the cutoff. The other 98.6% are measured and thrown away;
that observation drives the whole parallel course.

Now the repo's benchmark, which builds the same system with `LJParticleBoxBuilder` and computes the energy
with every backend (the project must be built first; see `CLAUDE.md`):

```bash
build/benchmarks/bench_lj 4096 3          # from the repo root
```
```
Creating LJ particle box: N=4096, rho=0.8, L=17.2355
Placed 4096 particles on cubic lattice

N = 4096   rho = 0.80   L = 17.235   cutoff = 2.5   reps = 3
OpenMP: yes  (default threads = 8)

backend               ms/call    vs ref           energy      max|df|      max rel
reference              34.951     1.00x -20105.493949019            -            -
serial-triangular      15.236     2.29x -20105.493949019    1.137e-13    1.490e-11
...
```

`-20105.493949019` printed to 14 digits; mini_lj printed `-20105.493949` to 12. Every digit mini_lj
shows matches. Seventy-five lines of one-file C++ reproduce the repo's reference energy, because they
build the same lattice from the same seed and run the same triangular loop with the same formula. The
`reference` row is `LJPotential::calculateEnergyReference` — the loop chapter 10 shows *is* mini_lj — and
the other rows are the parallel course's faster versions of it.

---

## 4.11 In the real code

### Where the atoms live: `Geometry`

`src/builder/geometry.h:30-32`, the data inside the repo's `Geometry` class:

```cpp
30      std::vector<std::string> atomTypes;
31      std::vector<std::array<double, 3>> atomCoordinates;
32      std::array<double, 3> boxDimensions = {0.0, 0.0, 0.0};
```

mini_lj's `types`, `positions` and `box` — except that the box is three lengths (x, y, z) instead of one,
so it is a `std::array<double, 3>`, given a value at creation as chapter 2 recommends. Line 31 is the AoS
layout from §4.3. Lines 19–20 of the same header hand these out as `const std::vector<…>&` — chapter 3's
"read it, do not copy it" — which is what `lj_potential.cpp:272-274` received with `const auto&`.

### The lattice: `createCubicLattice`

`src/builder/lj_particle_box_builder.cpp:29-50`, against mini_lj's lines 25–40:

```cpp
29      int particlesPerDim = static_cast<int>(std::cbrt(config.numParticles) + 0.5);
30      double spacing = boxLength / particlesPerDim;
...
33      std::mt19937 rng(config.randomSeed);
34      std::uniform_real_distribution<double> smallDisp(-0.05 * spacing, 0.05 * spacing);
...
37      std::size_t particlesAdded = 0;
38      for (int i = 0; i < particlesPerDim && particlesAdded < config.numParticles; ++i) {
39          for (int j = 0; j < particlesPerDim && particlesAdded < config.numParticles; ++j) {
40              for (int k = 0; k < particlesPerDim && particlesAdded < config.numParticles; ++k) {
41                  // Position on lattice, centered in box
42                  double x = (i + 0.5) * spacing - boxLength / 2.0 + smallDisp(rng);
43                  double y = (j + 0.5) * spacing - boxLength / 2.0 + smallDisp(rng);
44                  double z = (k + 0.5) * spacing - boxLength / 2.0 + smallDisp(rng);
45
46                  geometry.addAtom("LJ", {x, y, z});
47                  ++particlesAdded;
48              }
49          }
50      }
```

| Repo | mini_lj | Note |
|---|---|---|
| 29 `particlesPerDim` | 25 `perSide` | same `+ 0.5` then `static_cast<int>` |
| 33 `rng(config.randomSeed)` | 27 `rng(42)` | the benchmark's seed is 42 |
| 37, 47 `particlesAdded` counter | `positions.size()` | the repo counts by hand because `Geometry` is not a vector |
| 38–40 | 32–34 | same loops; the repo writes the braces |
| 42–44 | 35–37 | same formula, same order of random draws |
| 46 `geometry.addAtom("LJ", {x, y, z})` | 38–39, two `push_back`s | `addAtom` pushes onto both of `Geometry`'s vectors |

### The map lookup, guarded

`src/engine/potentials/lj_potential.cpp:335-342`, the repo's version of mini_lj's lines 60–61:

```cpp
335  std::pair<double, double> LJPotential::getMixedParameters(const std::string& type1, const std::string& type2) {
336      if (atomParameters.find(type1) == atomParameters.end() ||
337          atomParameters.find(type2) == atomParameters.end()) {
338          return {0.0, 0.0};  // Return zero parameters if missing
339      }
340
341      const auto& params1 = atomParameters[type1];
342      const auto& params2 = atomParameters[type2];
```

Lines 336–337 are §4.6's `.find(k) == .end()` test, done *first*, so the `[]` on lines 341–342 can never
insert. (`||` is logical "or".) The function returns two numbers as a `std::pair` — chapter 5. The map
itself is declared at `lj_potential.h:70`: `std::map<std::string, AtomParameters> atomParameters;`. The
comment above `calculateEnergyReference` in the header (`lj_potential.h:57`) calls this lookup out:
"it does a std::map<std::string> lookup per pair".

### Preparing an output vector

`src/engine/potentials/lj_potential.cpp:301-307`, at the top of the reference force calculation:

```cpp
301      if (forces.size() != geometry.getAtoms().size()) {
302          forces.resize(geometry.getAtoms().size(), {0.0, 0.0, 0.0});
303      } else {
304          for (auto& force : forces) {
305              force = {0.0, 0.0, 0.0};
306          }
307      }
```

`forces` is the out-parameter from chapter 3 (`std::vector<std::array<double, 3>>& forces`). If the
caller's vector has the wrong length, line 302 resizes it, filling new entries with zero forces. Otherwise
lines 304–306 zero every entry in place — range-for with `auto&` (not `const`, not a copy), because the
loop's job is to change the elements. Then the pair loop adds into `forces[i]` and `forces[j]`.

---

## 4.12 When a vector grows, its elements move (deep dive later)

A `std::vector` keeps its elements in one block of memory. When `push_back` needs more room than the block
has, the vector allocates a bigger block, copies (or moves) every element across, and frees the old block.
From `vector_demo.cpp`:

```cpp
std::vector<double> grow;
grow.push_back(0.0);
const double* before = grow.data();
for (int i = 0; i < 100; ++i) grow.push_back(i);
const double* after = grow.data();
std::cout << "storage moved while growing: " << (before != after ? "yes" : "no") << std::endl;
```
```
storage moved while growing: yes
```

`before` still holds the old address, but nothing valid lives there any more. A pointer like that is
called **dangling**; following it is undefined behaviour. The same is true of a reference to an element
(`const Params& pi = …` style) or the result of `.data()` taken before the growth.

This is why the repo's parallel path sizes its vectors first and only *then* takes `.data()`:
`LJPotential::buildView` calls `sx.resize(n)` and friends at `lj_potential.cpp:99-101` and hands out
`sx.data()` at line 136 — chapter 3's §3.9. Chapter 8 (ownership and lifetime) returns to exactly this.

---

## Next

mini_lj now gets the right answer — the repo's reference energy for 4096 atoms — but look at its
shape. Everything is one `main`, lines 18 to 75: reading N, building the lattice, filling the parameter
table, the pair loop, the minimum image, the mixing rule and the LJ formula, all tangled together. You
cannot test the pair energy on its own, or reuse the lattice anywhere else. Chapter 5 pulls the maths out
into named functions in their own files — `lj_math.{h,cpp}` and `lattice.{h,cpp}` — using chapter 1's
declaration/definition split for real.
