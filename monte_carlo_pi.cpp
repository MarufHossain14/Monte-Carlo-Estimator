#include <algorithm>
#include <charconv>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <random>
#include <string>
#include <utility>
#include <vector>

namespace {

constexpr double kPi = 3.14159265358979323846;

struct EstimatePoint {
    std::uint64_t sample_count;
    double pi_estimate;
};

struct EstimationResult {
    std::uint64_t sample_count;
    std::uint64_t points_inside_circle;
    std::uint32_t seed;
    double pi_estimate;
    std::vector<EstimatePoint> intermediate_estimates;
};

using ProgressCallback =
    std::function<void(std::uint64_t completed, double current_estimate)>;

class MonteCarloPiEstimator {
public:
    explicit MonteCarloPiEstimator(std::uint32_t seed)
        : seed_(seed), engine_(seed), coordinate_(-1.0, 1.0) {}

    /**
     * Run the simulation without performing console or file output.
     *
     * Callers decide how results are presented. An optional callback provides
     * progress updates without coupling the simulation to a terminal.
     */
    [[nodiscard]] EstimationResult estimate(
        std::uint64_t sample_count,
        bool collect_intermediate_estimates,
        const ProgressCallback& progress_callback = {}) {
        std::uint64_t points_inside = 0;
        const std::uint64_t progress_step =
            std::max<std::uint64_t>(1, sample_count / 10);
        const std::uint64_t estimate_step =
            std::max<std::uint64_t>(1, sample_count / 1000);

        std::vector<EstimatePoint> intermediate_estimates;
        if (collect_intermediate_estimates) {
            intermediate_estimates.reserve(1001);
        }

        for (std::uint64_t index = 0; index < sample_count; ++index) {
            const double x = coordinate_(engine_);
            const double y = coordinate_(engine_);

            if (x * x + y * y <= 1.0) {
                ++points_inside;
            }

            const std::uint64_t completed = index + 1;
            const double current_estimate =
                4.0 * static_cast<double>(points_inside) /
                static_cast<double>(completed);

            if (progress_callback &&
                (completed % progress_step == 0 ||
                 completed == sample_count)) {
                progress_callback(completed, current_estimate);
            }

            if (collect_intermediate_estimates &&
                (completed % estimate_step == 0 ||
                 completed == sample_count)) {
                intermediate_estimates.push_back({completed, current_estimate});
            }
        }

        const double final_estimate =
            4.0 * static_cast<double>(points_inside) /
            static_cast<double>(sample_count);

        return {
            sample_count,
            points_inside,
            seed_,
            final_estimate,
            std::move(intermediate_estimates),
        };
    }

private:
    std::uint32_t seed_;
    std::mt19937 engine_;
    std::uniform_real_distribution<double> coordinate_;
};

void print_result(
    const EstimationResult& result,
    std::chrono::duration<double, std::milli> elapsed) {
    const double absolute_error = std::abs(result.pi_estimate - kPi);

    std::cout << "\n=== Monte Carlo Pi Estimation Results ===\n"
              << "Number of samples: " << result.sample_count << '\n'
              << "Points inside circle: "
              << result.points_inside_circle << '\n'
              << "Random seed: " << result.seed << '\n'
              << std::fixed << std::setprecision(10)
              << "Estimated pi: " << result.pi_estimate << '\n'
              << "Actual pi: " << kPi << '\n'
              << "Absolute error: " << absolute_error << '\n'
              << std::setprecision(6)
              << "Relative error: " << (absolute_error / kPi) * 100.0
              << "%\n"
              << std::setprecision(3)
              << "Computation time: " << elapsed.count() << " ms\n";
}

bool write_csv(
    const std::string& filename,
    const EstimationResult& result,
    bool include_intermediate_estimates) {
    std::ofstream file(filename);
    if (!file) {
        std::cerr << "Error: Could not open file " << filename << '\n';
        return false;
    }

    file << "Sample_Count,Pi_Estimate,Error,Seed\n";
    file << std::fixed << std::setprecision(10);

    if (include_intermediate_estimates) {
        for (const auto& estimate : result.intermediate_estimates) {
            file << estimate.sample_count << ',' << estimate.pi_estimate << ','
                 << std::abs(estimate.pi_estimate - kPi) << ','
                 << result.seed << '\n';
        }
    } else {
        file << result.sample_count << ',' << result.pi_estimate << ','
             << std::abs(result.pi_estimate - kPi) << ',' << result.seed
             << '\n';
    }

    if (!file) {
        std::cerr << "Error: Failed while writing " << filename << '\n';
        return false;
    }

    std::cout << "Results saved to " << filename << '\n';
    return true;
}

void print_usage(const char* program_name) {
    std::cout << "Usage: " << program_name << " [options]\n"
              << "Options:\n"
              << "  -n <number>    Number of samples (default: 1000000)\n"
              << "  -p             Show progress during computation\n"
              << "  --seed <number> Use a reproducible 32-bit random seed\n"
              << "  -s <filename>  Save results to a CSV file\n"
              << "  -i             Include intermediate estimates in the CSV\n"
              << "  -h, --help     Show this help message\n\n"
              << "Examples:\n"
              << "  " << program_name << " -n 1000000 --seed 42 -p\n"
              << "  " << program_name
              << " -n 10000000 -s results.csv -i\n";
}

template <typename UnsignedInteger>
bool parse_unsigned_integer(
    const std::string& text,
    UnsignedInteger& parsed_value) {
    if (text.empty() || text.front() == '-') {
        return false;
    }

    const char* begin = text.data();
    const char* end = begin + text.size();
    const auto result = std::from_chars(begin, end, parsed_value);
    return result.ec == std::errc{} && result.ptr == end;
}

int option_error(const std::string& message, const char* program_name) {
    std::cerr << "Error: " << message << "\n\n";
    print_usage(program_name);
    return 1;
}

}  // namespace

int main(int argc, char* argv[]) {
    std::uint64_t sample_count = 1'000'000;
    std::uint32_t seed = std::random_device{}();
    bool show_progress = false;
    bool save_to_file = false;
    bool include_intermediate_estimates = false;
    std::string filename = "monte_carlo_pi_results.csv";

    for (int index = 1; index < argc; ++index) {
        const std::string argument = argv[index];

        if (argument == "-h" || argument == "--help") {
            print_usage(argv[0]);
            return 0;
        }
        if (argument == "-n") {
            if (index + 1 >= argc) {
                return option_error("-n requires a value", argv[0]);
            }
            if (!parse_unsigned_integer(argv[++index], sample_count) ||
                sample_count == 0) {
                return option_error(
                    "sample count must be a positive integer", argv[0]);
            }
        } else if (argument == "-p") {
            show_progress = true;
        } else if (argument == "--seed") {
            if (index + 1 >= argc) {
                return option_error("--seed requires a value", argv[0]);
            }
            if (!parse_unsigned_integer(argv[++index], seed)) {
                return option_error(
                    "seed must be a 32-bit unsigned integer", argv[0]);
            }
        } else if (argument == "-s") {
            if (index + 1 >= argc) {
                return option_error("-s requires a filename", argv[0]);
            }
            filename = argv[++index];
            if (filename.empty()) {
                return option_error("CSV filename cannot be empty", argv[0]);
            }
            save_to_file = true;
        } else if (argument == "-i") {
            include_intermediate_estimates = true;
        } else {
            return option_error("unknown option: " + argument, argv[0]);
        }
    }

    if (include_intermediate_estimates && !save_to_file) {
        return option_error("-i requires -s <filename>", argv[0]);
    }

    std::cout << "Monte Carlo Pi Estimator\n"
              << "========================\n";

    ProgressCallback progress_callback;
    if (show_progress) {
        progress_callback = [sample_count](
                                std::uint64_t completed,
                                double current_estimate) {
            std::cout << "Progress: " << std::fixed << std::setprecision(1)
                      << 100.0 * static_cast<double>(completed) /
                             static_cast<double>(sample_count)
                      << "% - Current pi estimate: " << std::setprecision(6)
                      << current_estimate << '\n';
        };
    }

    MonteCarloPiEstimator estimator(seed);
    const auto start = std::chrono::steady_clock::now();
    const auto result = estimator.estimate(
        sample_count, include_intermediate_estimates, progress_callback);
    const auto elapsed = std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now() - start);

    print_result(result, elapsed);

    if (save_to_file &&
        !write_csv(filename, result, include_intermediate_estimates)) {
        return 1;
    }

    return 0;
}
