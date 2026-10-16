# Monte Carlo Estimator

This C++17 project is being developed from a small Monte Carlo pi estimator
into a tested particle-transport simulation and performance investigation.

The pi estimator remains as an introductory example. The particle-transport
core models transmission, reflection, absorption, and scattering in a
one-dimensional slab. Its assumptions are described in
[the particle model documentation](docs/particle_model.md).

## Current components

- `monte_carlo_pi`: a command-line Monte Carlo pi estimator with reproducible
  seeds, progress reporting, precise timing, and CSV export.
- `particle_transport`: the command-line particle simulation, backed by a
  reusable C++ library.
- `particle_transport_tests`: correctness tests for the initial transport
  model.
- `plot_results.py`: an optional Python script for visualizing pi-estimator
  CSV output.

## How the pi estimator works

The program samples points uniformly from the square `[-1, 1] x [-1, 1]`.
A point is inside the unit circle when:

```text
x*x + y*y <= 1
```

The unit circle has area pi and the square has area 4. Therefore, the fraction
of sampled points inside the circle approaches `pi / 4`, giving the estimator:

```text
pi_estimate = 4 * points_inside / total_points
```

Monte Carlo error fluctuates between runs; it does not decrease monotonically.
Its typical scale decreases in proportion to `1 / sqrt(N)`, so obtaining an
additional decimal digit generally requires about 100 times as many samples.

## Requirements

- A C++17 compiler
- CMake 3.10 or newer
- Make, if using the included Makefile instead of CMake
- Python 3 with the packages in `requirements.txt`, only for plotting

## Build with CMake

Configure an optimized build from the repository root:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

On Linux and macOS, the pi executable is:

```bash
./build/bin/monte_carlo_pi
```

With a multi-configuration generator such as Visual Studio, include the
configuration while building:

```powershell
cmake -S . -B build
cmake --build build --config Release
.\build\bin\Release\monte_carlo_pi.exe
```

## Run the tests

After building with CMake:

```bash
ctest --test-dir build --output-on-failure
```

## Particle-transport usage

Run the simulation with its documented default configuration:

```bash
./build/bin/particle_transport
```

Configure the number of particles and material properties explicitly:

```bash
./build/bin/particle_transport \
    --particles 100000 \
    --thickness 2.0 \
    --absorption 0.4 \
    --scattering 0.6 \
    --seed 42 \
    --max-collisions 100000 \
    --csv transport_results.csv
```

The program reports transmitted, reflected, and absorbed counts; their
estimated probabilities and 95% confidence intervals; the number of particles
that scattered; total and mean collisions; mean path length; and execution
time. The optional CSV file contains the model configuration and results in a
single row. Timing is intentionally excluded from the CSV so identical inputs
and seeds produce identical files.

Use `./build/bin/particle_transport --help` to see all options. The simplified
physics and terminology are explained in
[the particle model documentation](docs/particle_model.md).

## Alternative pi-estimator builds

Using the Makefile:

```bash
make
./monte_carlo_pi
```

Using GCC directly:

```bash
g++ -std=c++17 -O3 -Wall -Wextra -Wpedantic \
    monte_carlo_pi.cpp -o monte_carlo_pi
./monte_carlo_pi
```

Windows users with `g++` available in `PATH` can also run
`build_and_run.bat`.

## Pi-estimator usage

```text
Usage: monte_carlo_pi [options]
Options:
  -n <number>    Number of samples (default: 1000000)
  -p             Show progress during computation
  --seed <number> Use a reproducible 32-bit random seed
  -s <filename>  Save results to a CSV file
  -i             Include intermediate estimates in the CSV
  -h, --help     Show the help message
```

Examples:

```bash
# A reproducible run
./build/bin/monte_carlo_pi -n 1000000 --seed 42

# Display progress
./build/bin/monte_carlo_pi -n 10000000 --seed 42 -p

# Save the final result
./build/bin/monte_carlo_pi -n 1000000 --seed 42 -s result.csv

# Save intermediate convergence data
./build/bin/monte_carlo_pi -n 1000000 --seed 42 \
    -s convergence.csv -i
```

When no seed is supplied, the program generates one and prints it with the
result. Pass that value to `--seed` to reproduce the same random sequence.

The `-i` option is meaningful only with `-s`, so using it alone is rejected.

## CSV format

CSV output contains:

- `Sample_Count`: samples included in the estimate;
- `Pi_Estimate`: the estimate at that sample count;
- `Error`: absolute difference from pi;
- `Seed`: the random seed used for the run.

Without `-i`, the file contains one final result. With `-i`, it contains at
most about 1,000 intermediate estimates plus the final estimate.

## Plot results

Install the optional plotting dependencies:

```bash
python3 -m pip install -r requirements.txt
```

Generate data and create a convergence plot:

```bash
./build/bin/monte_carlo_pi -n 1000000 --seed 42 \
    -s convergence.csv -i
python3 plot_results.py convergence.csv -o convergence.png
```

## Reproducibility

When reporting results, record at least:

- the random seed and sample count;
- build type and compiler flags;
- compiler name and version;
- processor and operating system.

Progress output performs terminal I/O during the measured run. Disable it for
performance measurements.

## License

This project is available under the [MIT License](LICENSE).
