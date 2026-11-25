# APPSTSGenerate

APPSTSGenerate is a C17 command-line program that generates anti-Pasch partial
Steiner triple systems (PSTS) up to isomorphism. A PSTS is a collection of
three-point blocks in which each pair of points occurs in at most one block.
The program uses backtracking and canonicity checks to find designs, then
reports each design's automorphism group size, number of distinct mitres, and
number of distinct labeled designs.

The executable runs locally in one process on one thread. No server, network
connection, or Python runtime is required to run the generator.

## Requirements

- CMake 3.20 or newer.
- A C17 compiler, such as Clang, GCC, or MSVC, and its build tools.
- Python 3 to run the CLI regression tests (optional for building and running
  the executable).

CMake is the only supported build entry point. It can generate build files for
Ninja, Make, Visual Studio, or another available backend.

## Build and run

From the repository root:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
./build/antipasch_psts -ist 9
```

These commands use a separate `build` directory. With a multi-configuration
backend such as Visual Studio, the executable is in the configuration
subdirectory; for example, on Windows run:

```powershell
.\build\Release\antipasch_psts.exe -ist 9
```

The `--parallel` build option compiles source files concurrently; the generator
itself runs sequentially.

For CLion, open the repository as a CMake project using `CMakeLists.txt`, choose
a Release or Debug CMake profile, and build the `antipasch_psts` target. No IDE
settings need to be checked into the repository.

## Output and search behavior

The 9-point example produces this design line, followed by statistics and CPU
time because `-s` and `-t` were selected:

```text
Design: {0, 1, 2} {0, 3, 4} {0, 5, 6} {0, 7, 8} {1, 3, 5} {1, 4, 7} {1, 6, 8} {2, 3, 8} {2, 4, 6} {2, 5, 7} {3, 6, 7} {4, 5, 8} (automsize: 432, mitres: 36, #DistDesigns: 840)
```

Point labels are zero-based. `automsize` is the size of the design's
automorphism group, and `mitres` is its number of distinct five-block mitre
configurations. A mitre consists of three blocks `{a,b,c}`, `{a,d,e}`,
`{a,f,g}` through a centre `a`, and transversals `{b,d,f}` and `{c,e,g}`,
on seven distinct points. Each configuration is counted once, even when
contained in a larger design.

`#DistDesigns` is `v! / automsize`, the number of distinct labeled designs in
this isomorphism class by the orbit-stabilizer theorem. Before reporting a
design, the program checks that the group size is positive and divides `v!`
exactly; a violation prints an error and terminates with a nonzero exit status.
Counts use exact integer arithmetic with no extra library dependency, including
for point counts above 20 where the factorial exceeds `uint64_t`.

The last line of a successful search is `Total #DistDesigns: N`, summing the labeled counts of
all reported designs, independently of `-s`. For STS(9), this is 840. A search
with no designs reports zero. The total includes any smaller designs printed
before the lower block bound increases; it is a total over the reported
classes, not just those with the final maximum block count.

The default replication and block bounds depend on the point count. As larger
designs are discovered, the search raises its lower block bound; previously
printed smaller designs remain in the output. Some bounds admit no designs,
in which case no `Design:` lines are printed. Large searches can take
substantial time.

## Command-line options

Run `./build/antipasch_psts --help` for the options enabled in your build.
Short flags can be combined, as in `-ist`.

| Option | Meaning |
| --- | --- |
| `-r N`, `-R N` | Lower and upper replication bounds (blocks per point). |
| `-b N`, `-B N` | Lower and upper bounds on the number of blocks. |
| `-C RANGE` | Zero-based rows on which to check canonicity. |
| `-i` | Print initial bounds. |
| `-s` | Print generation statistics; requires `APPSTS_STATS=ON`. |
| `-t` | Print CPU time; requires `APPSTS_STATS=ON`. |
| `-e` | Print the canonicity processing count; requires `APPSTS_STATS=ON`. |
| `-p`, `-c`, `-q` | Print partial matrices, complete matrices, and zero rows, respectively; Debug builds only. |
| `-h`, `--help` | Show usage. |

`-C` accepts individual rows, inclusive ranges, and open ranges; for example,
`-C '-2,4,6-'`. The final row is always checked. By default, rows 3 onward and
the final row are checked.

The point count must be between 3 and 64. This is a representation limit,
not a guarantee that large searches will finish quickly. Replication bounds
and the upper block bound must be positive and within the combinatorial
limits. The lower block bound may be zero. Every point must occur in at
least one block.

## Tests and debug builds

After building, run:

```sh
ctest --test-dir build -C Release --output-on-failure
```

`BUILD_TESTING` defaults to `ON`. C helper tests cover bit operations at all 64
positions, triple-index round trips, mitre fixtures, exact labeled counts, and
Lagrange checks (including arithmetic through `64!`). If Python 3 is
available at configuration time, CLI tests also check argument validation,
pair uniqueness, absence of Pasch configurations, automorphism counts, and
mitre counts using independent enumeration, and labeled counts and their sum.
CMake reports when the Python
tests are unavailable.

For a Debug build with assertions, matrix output options, and address/undefined
behavior sanitizers (Clang or GCC):

```sh
cmake -S . -B build-debug -DCMAKE_BUILD_TYPE=Debug -DAPPSTS_SANITIZERS=ON
cmake --build build-debug --config Debug --parallel
ctest --test-dir build-debug -C Debug --output-on-failure
```

GitHub Actions is configured to build and test Linux, macOS, and Windows
releases, plus Linux sanitizer and alternate-feature builds.

## CMake options

Pass options at configuration time with `-DNAME=VALUE`.

| Option | Default | Meaning |
| --- | --- | --- |
| `APPSTS_STATS` | `ON` | Collect statistics and enable `-s`, `-t`, and `-e`. |
| `APPSTS_STATIC_LEX` | `ON` | Precompute triple lookup tables; `OFF` calculates indices instead. |
| `APPSTS_SANITIZERS` | `OFF` | Enable address and undefined behavior sanitizers; requires Clang or GCC. |
| `BUILD_TESTING` | `ON` | Build and register the tests. |

To build just the executable without tests, configure with
`-DBUILD_TESTING=OFF`.

## Install and clean

To install the executable into a chosen prefix:

```sh
cmake --install build --config Release --prefix /your/install/path
```

The executable is installed under `bin` in that prefix, and the license under
`share/licenses/APPSTSGenerate`. To remove compiled
outputs while keeping the configuration, run
`cmake --build build --config Release --target clean`. To start from scratch,
remove the build directory and configure again. Generated build directories
and personal IDE settings are excluded by `.gitignore`.

## License

BSD 3-Clause; see [LICENSE](LICENSE).
