# Bridge Solver X-ray

An instrumented version of [macroxue's bridge-solver](https://github.com/macroxue/bridge-solver) with enhanced debugging and tracing capabilities.

## Purpose

This repository maintains an instrumented ("X-ray") version of the bridge double dummy solver for:

- **Debugging**: Trace solver execution with detailed logging of search states, move ordering, alpha-beta cutoffs, and transposition table operations
- **Validation**: Verify that modified or reimplemented solvers produce identical results to the reference implementation
- **Understanding**: Study the solver's decision-making process for educational purposes

## Repository Structure

- **Root**: Original upstream solver files (unmodified)
- **xray/**: Instrumented solver, tests, and build files

## Building

```bash
cd xray
make
```

This builds `solver-xray` from `bridge-solver.cc`.

## Running Tests

```bash
cd xray
make test
```

Runs the solver against 100 test deals and compares results to golden reference output.

## X-ray Debugging Options

The instrumented solver supports additional command-line options:

| Option | Description |
|--------|-------------|
| `-X N` | Enable X-ray tracing for first N SearchWithCache calls |
| `-P`   | Disable pruning (fast/slow tricks optimization) |
| `-T`   | Disable transposition table |
| `-R`   | Disable min_relevant_ranks optimization |

### Example: Trace first 10 search calls

```bash
cd xray
./solver-xray -f test_deals/deal.1 -X 10
```

### Logging Categories

When X-ray tracing is enabled, the solver outputs detailed logs:

- **XRAY**: Entry point with hands and play history
- **FAST_TRICKS / SLOW_TRICKS**: Pruning calculations
- **MOVE_ORDER**: Card ordering before/after evaluation
- **SCORE**: Individual move scores during search
- **CUTOFF**: Alpha-beta cutoff events
- **EQUIV_V2**: Card equivalence checking

## Upstream

This repository tracks [macroxue/bridge-solver](https://github.com/macroxue/bridge-solver).

---

## Original Documentation

Below is the original README from the upstream solver.

---

# Bridge double dummy solver

This is a fairly simple and yet effective double dummy solver for the card
game of bridge. It's terminal based.

## Solve a random deal
```
cd xray
./solver-xray -r
```
The output looks like below.
```
                          ♠ KJT987 ♥ K5 ♦ 7 ♣ AQJ8
  ♠ 3 ♥ J9764 ♦ Q642 ♣ KT2                       ♠ Q64 ♥ QT8 ♦ KJ953 ♣ 94
                          ♠ A52 ♥ A32 ♦ AT8 ♣ 7653
N 13 13  0  0  0.01 s  10.2 M
S 13 13  0  0  0.02 s  10.2 M
H  7  7  6  5  0.21 s  12.9 M
D  6  6  6  6  0.34 s  13.2 M
C 13 13  0  0  0.34 s  13.2 M
```
Each line after the deal shows the strain to play, the number of tricks when
South/North/West/East declares respectively, the cumulative time and the peak
memory usage.

## Solve a deal in a file

```
cd xray
./solver-xray -f FILE
```

The format of the deal in the file is like below.
```
               KQ3 - T832 AJ9765
72 AJ972 AQ7 KQ2               T96 K83 654 T843
               AJ854 QT654 KJ9 -
D
W
```
The first line is North. The second line has both West and East. The third line
is South. The forth line specifies the strain to play. The fifth line is the
leading seat. If the leading seat is not given, the deal is solved for all four
leading seats. If the strain to play is also not given, the deal is solved for
all five strains.

## Interactive play
```
cd xray
./solver-xray -r -p
```
or
```
./solver-xray -f FILE -p
```

The solver automatically determines the contract. If nobody can make any
contract, the hand is skipped. For each turn, the solver evaluates each of the
player's card and shows the result of the contract if the card is played and
the rest is played by everyone optimally. A sign is shown next to each card
with the following meanings.
| Sign | Meaning |
|------|---------|
|  =   | The contract makes. |
|  +   | The contract gets an overtrick. |
|  -   | The contract is set by a trick. |
| (+N) | The contract gets N overtricks. |
| (-N) | The contract is set by N tricks. |

## Performance

The original solver benchmarks are available in the upstream repository.
