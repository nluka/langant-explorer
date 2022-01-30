# Ant Simulator Lite

High-performance application for running customized [Langton's Ant](https://en.wikipedia.org/wiki/Langton%27s_ant) simulations.

- [Usage](#usage)
  - [Options](#options)
    - [--image-output-path, -o](#--image-output-path--o)
    - [--log, -l](#--logs--l)
    - [--no-image-output, -n](#--no-image-output--n)
- [Simulation File](#simulation-file)
  - [Format](#format)
    - [Notes](#notes)
  - [Properties](#properties)
  - [Examples](#examples)
    - [Classic](#classic)
    - [Classic + Inverted](#classic--inverted)

## Usage

The first argument must be the pathname to the [simulation file](#simulation-file) for the program to use. All other arguments are options. Use `-v`/`--version` to check the program version.

### Options

#### --image-output-path, -o

Specifies the output path for image files.

For example: `-o ../out` or `--image-output-path out/`.

#### --log, -l

Specifies the pathname to the file in which to save event logs.

For example: `-l sample.log` or `--log log.txt`.

#### --no-image-output, -n

Flag that disables image output. Useful for benchmarking.

## Simulation File

Defines the simulation(s) for the program to run, can be any text file.

### Format

    (simulation name, simulation output-format, simulation max-iterations)
    (grid width, grid height, grid initial-color)
    [ (rule color, rule turn-direction rule replacement-color)... ]
    (ant inital-column, ant inital-row, ant initial-orientation)

### Properties

| Property name | Type | Possible Values | Description |
| ------------- | ---- | --------------- | ----------- |
| `simulation name` | string | [/^[a-zA-Z0-9-_]{1,64}$/](https://regexr.com/67onu) | Determines the name of the emitted image and log file. |
| `simulation output-format` | string | [/^(PGMa)\|(PGMb)$/](https://regexr.com/67oo4) | Determines the file format of the emitted image. 'PGMa' = ASCII PGM, 'PGMb' = binary PGM. |
| `simulation max-iterations` | int | 0 to (2^64 - 1) | Sets a limit for the number of simulation iterations. 0 indicates no limit. Simulations with no limit can run forever. |
| `grid width` | int | 0 to 65,535 | The number of columns in the simulation's grid. |
| `grid height` | int | 0 to 65,535 | The number of rows in the simulation's grid. |
| `grid initial-color` | int | 0 to 255 | The initial color value of all grid cells. |
| `rule color` | int | 0 to 255 | The color value of cells the rule applies to. |
| `rule turn-direction` | string | [/^(left)\|(right)\|(none)$/](https://regexr.com/67oo7) | The direction the ant will turn before stepping off a grid cell that is governed by the current rule. |
| `rule replacement-color` | int | 0 to 255 | The color value that will replace the current color value before the ant steps off a grid cell governed by the current rule. |
| `ant initial-column` | int | 0 to (`grid width` - 1) | The starting column of the ant. |
| `ant initial-row` | int | 0 to (`grid height` - 1) | The starting row of the ant. |
| `ant initial-orientation` | string | [/(north)\|(east)\|(south)\|(west)/](https://regexr.com/67ooa) | The starting orientation of the ant. |

#### Notes

- A simulation can have multiple rules
- All rules must be governed by another rule
- Grid initial-color must be governed by a rule
- Simulations are delimited by '#' and up to 8 can be specified

### Examples

#### Classic

    simulation: (classic, PGMa, 11000)
    grid: (200, 200, 1)
    rules: [
      (1, right, 0)
      (0, left, 1)
    ]
    ant: (100, 100, north)

#### RLR

    simulation: (rlr, PGMb, 50000)
    grid: (200, 200, 1)
    rules: [
      (0, right, 1)
      (1, left, 2)
      (2, right, 0)
    ]
    ant: (100, 100, east)

#### LLRR

    simulation: (llrr, PGMa, 50000)
    grid: (200, 200, 0)
    rules: [
      (0, left, 1)
      (1, left, 2)
      (2, right, 3)
      (3, right, 0)
    ]
    ant: (100, 100, south)

#### LRRRRRLLR

    simulation: (lrrrrrllr, PGMb, 1000000)
    grid: (1000, 1000, 1)
    rules: [
      (1, left, 2)
      (2, right, 3)
      (3, right, 4)
      (4, right, 5)
      (5, right, 6)
      (6, right, 7)
      (7, left, 8)
      (8, left, 9)
      (9, right, 1)
    ]
    ant: (500, 500, west)

#### LLRRRLRLRLLR

    simulation: (llrrrlrlrllr, PGMa, 100000)
    grid: (1000, 100, 0)
    rules: [
      (0, left, 1)
      (1, left, 2)
      (2, right, 3)
      (3, right, 4)
      (4, right, 5)
      (5, left, 6)
      (6, right, 7)
      (7, left, 8)
      (8, right, 9)
      (9, left, 10)
      (10, left, 11)
      (11, right, 0)
    ]
    ant: (50, 50, south)

#### RRLLLRLLLRRR

    simulation: (rrlllrlllrrr, PGMb, 0)
    grid: (5000, 5000, 0)
    rules: [
      (0, right, 1)
      (1, right, 2)
      (2, left, 3)
      (3, left, 4)
      (4, left, 5)
      (5, right, 6)
      (6, left, 7)
      (7, left, 8)
      (8, left, 9)
      (9, right, 10)
      (10, right, 11)
      (11, right, 0)
    ]
    ant: (2500, 2500, west)