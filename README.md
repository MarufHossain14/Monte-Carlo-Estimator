# Monte Carlo π Estimator

A C++ program that estimates the value of π using Monte Carlo simulation. The program generates random points within a square of side length 2 and counts how many fall inside the inscribed unit circle to compute an approximation of π.

## Mathematical Background

The Monte Carlo method for estimating π works as follows:

1. Consider a square with side length 2 centered at the origin
2. Inscribe a unit circle (radius 1) within the square
3. Generate random points uniformly distributed within the square
4. Count the ratio of points that fall inside the circle
5. Since the area of the circle is πr² = π and the area of the square is 4, the ratio of points inside the circle to total points approaches π/4
6. Therefore, π ≈ 4 × (points inside circle) / (total points)

## Features

- **Modern C++17**: Uses `std::mt19937` random number generator for high-quality randomness
- **High Performance**: Optimized for speed with efficient algorithms
- **Progress Tracking**: Optional real-time progress display during computation
- **Command Line Interface**: Flexible command-line options for customization
- **CSV Export**: Save results to CSV files for analysis and plotting
- **Error Analysis**: Calculates absolute and relative errors compared to actual π
- **Timing**: Measures and displays computation time
- **Intermediate Results**: Option to save intermediate estimates for convergence analysis

## Requirements

- C++17 compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)
- CMake 3.10+ (optional, for CMake build)
- Make (optional, for Makefile build)

## Building the Project

### Option 1: Using CMake (Recommended)

```bash
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

### Option 2: Using Makefile

```bash
make
```

### Option 3: Direct Compilation

```bash
g++ -std=c++17 -O3 -o monte_carlo_pi monte_carlo_pi.cpp -lm
```

## Usage

### Basic Usage

```bash
# Run with default settings (1,000,000 samples)
./monte_carlo_pi

# Run with progress display
./monte_carlo_pi -p

# Run with custom number of samples
./monte_carlo_pi -n 10000000 -p
```

### Command Line Options

- `-n <number>`: Number of samples (default: 1,000,000)
- `-p`: Show progress during computation
- `-s <filename>`: Save results to CSV file
- `-i`: Include intermediate estimates in CSV output
- `-h` or `--help`: Show help message

### Examples

```bash
# Quick test with 100,000 samples and progress
./monte_carlo_pi -n 100000 -p

# High-precision run with 10 million samples
./monte_carlo_pi -n 10000000 -p

# Save final result to CSV
./monte_carlo_pi -n 1000000 -s result.csv

# Save intermediate results for plotting
./monte_carlo_pi -n 1000000 -s convergence.csv -i
```

### Makefile Targets

If using the Makefile:

```bash
make run              # Run with default settings
make run-progress     # Run with progress display
make run-samples      # Run with 10M samples and progress
make run-csv          # Generate CSV with intermediate results
make clean            # Clean build files
```

## Output Format

The program displays:

- Number of samples used
- Points inside the circle
- Estimated value of π
- Actual value of π (for comparison)
- Absolute error
- Relative error (percentage)
- Computation time

Example output:

```
Monte Carlo π Estimator
=========================

=== Monte Carlo π Estimation Results ===
Number of samples: 1000000
Points inside circle: 785398
Estimated π: 3.1415920000
Actual π: 3.1415926536
Absolute error: 0.0000006536
Relative error: 0.000021%
Computation time: 45 ms
```

## CSV Output Format

When saving to CSV, the file contains:

- `Sample_Count`: Number of samples at this point
- `Pi_Estimate`: Current π estimate
- `Error`: Absolute error from actual π

## Performance Considerations

- **Sample Size**: More samples generally provide better accuracy, but take longer to compute
- **Random Number Quality**: Uses Mersenne Twister (MT19937) for high-quality randomness
- **Optimization**: Compiled with `-O3` for maximum performance
- **Memory Usage**: Minimal memory footprint, suitable for very large sample sizes

## Accuracy vs. Sample Size

Typical accuracy improvements:

- 10,000 samples: ~0.1% error
- 100,000 samples: ~0.03% error
- 1,000,000 samples: ~0.01% error
- 10,000,000 samples: ~0.003% error

## Visualization

The CSV output can be used with plotting tools like:

- Python (matplotlib, pandas)
- R (ggplot2)
- Excel or Google Sheets
- GNU Plot

Example Python plotting script:

```python
import pandas as pd
import matplotlib.pyplot as plt

# Load data
df = pd.read_csv('convergence.csv')

# Plot convergence
plt.figure(figsize=(10, 6))
plt.semilogx(df['Sample_Count'], df['Pi_Estimate'], 'b-', label='Estimated π')
plt.axhline(y=3.1415926536, color='r', linestyle='--', label='Actual π')
plt.xlabel('Number of Samples')
plt.ylabel('π Estimate')
plt.title('Monte Carlo π Convergence')
plt.legend()
plt.grid(True)
plt.show()
```

## License

This project is open source and available under the MIT License.

## Contributing

Feel free to submit issues, feature requests, or pull requests to improve the project.
