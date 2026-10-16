#include "transport/transport.hpp"

#include <algorithm>
#include <charconv>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#ifndef MONTE_CARLO_COMPILER_ID
#define MONTE_CARLO_COMPILER_ID "unknown"
#endif

#ifndef MONTE_CARLO_COMPILER_VERSION
#define MONTE_CARLO_COMPILER_VERSION "unknown"
#endif

#ifndef MONTE_CARLO_BUILD_TYPE
#define MONTE_CARLO_BUILD_TYPE "unknown"
#endif

#ifndef MONTE_CARLO_CXX_FLAGS
#define MONTE_CARLO_CXX_FLAGS "unknown"
#endif

#ifndef MONTE_CARLO_SYSTEM_NAME
#define MONTE_CARLO_SYSTEM_NAME "unknown"
#endif

#ifndef MONTE_CARLO_SYSTEM_PROCESSOR
#define MONTE_CARLO_SYSTEM_PROCESSOR "unknown"
#endif

namespace {

struct BenchmarkOptions {
    std::vector<std::uint64_t> particle_counts{
        1'000, 10'000, 100'000, 1'000'000};
    std::uint64_t repetitions{7};
    std::uint64_t warmup_runs{1};
    std::string output_filename{"transport_benchmark.csv"};
    transport::Material material{0.5, 0.5};
    transport::SimulationConfig simulation{};
    std::string cpu_label;
};

struct TimedRun {
    std::uint64_t particle_count;
    std::uint64_t repetition;
    double elapsed_milliseconds;
    double particles_per_second;
    transport::SimulationResults results;
};

struct Environment {
    std::string cpu;
    std::string operating_system;
    std::string architecture;
    std::string compiler_id;
    std::string compiler_version;
    std::string build_type;
    std::string compiler_flags;
};

void print_usage(const char* program_name) {
    std::cout
        << "Usage: " << program_name << " [options]\n\n"
        << "Benchmark the serial particle-transport simulation.\n\n"
        << "Options:\n"
        << "  --particles <list>       Comma-separated particle counts\n"
        << "                           (default: 1000,10000,100000,1000000)\n"
        << "  --repetitions <count>    Measured runs per workload (default: 7)\n"
        << "  --warmup <count>         Warm-up runs per workload (default: 1)\n"
        << "  --output <filename>      Raw CSV output path\n"
        << "                           (default: transport_benchmark.csv)\n"
        << "  --thickness <value>      Slab thickness (default: 1.0)\n"
        << "  --absorption <value>     Absorption coefficient (default: 0.5)\n"
        << "  --scattering <value>     Scattering coefficient (default: 0.5)\n"
        << "  --seed <number>          Fixed random seed (default: 5489)\n"
        << "  --max-collisions <count> Per-particle safety limit\n"
        << "  --cpu-label <text>       Override the detected CPU description\n"
        << "  -h, --help               Show this help message\n";
}

template <typename UnsignedInteger>
bool parse_unsigned(const std::string& text, UnsignedInteger& value) {
    if (text.empty() || text.front() == '-') {
        return false;
    }

    const char* begin = text.data();
    const char* end = begin + text.size();
    const auto result = std::from_chars(begin, end, value);
    return result.ec == std::errc{} && result.ptr == end;
}

bool parse_positive_double(const std::string& text, double& value) {
    std::size_t parsed_characters = 0;
    try {
        value = std::stod(text, &parsed_characters);
    } catch (const std::exception&) {
        return false;
    }

    return parsed_characters == text.size() && std::isfinite(value) &&
        value > 0.0;
}

bool parse_nonnegative_double(const std::string& text, double& value) {
    std::size_t parsed_characters = 0;
    try {
        value = std::stod(text, &parsed_characters);
    } catch (const std::exception&) {
        return false;
    }

    return parsed_characters == text.size() && std::isfinite(value) &&
        value >= 0.0;
}

bool parse_particle_counts(
    const std::string& text,
    std::vector<std::uint64_t>& particle_counts) {
    std::vector<std::uint64_t> parsed_counts;
    std::size_t start = 0;

    while (start <= text.size()) {
        const std::size_t comma = text.find(',', start);
        const std::size_t end =
            comma == std::string::npos ? text.size() : comma;
        const std::string item = text.substr(start, end - start);

        std::uint64_t count = 0;
        if (!parse_unsigned(item, count) || count == 0) {
            return false;
        }
        parsed_counts.push_back(count);

        if (comma == std::string::npos) {
            break;
        }
        start = comma + 1;
    }

    if (parsed_counts.empty()) {
        return false;
    }

    particle_counts = std::move(parsed_counts);
    return true;
}

bool parse_options(
    int argc,
    char* argv[],
    BenchmarkOptions& options,
    bool& show_help,
    std::string& error) {
    for (int index = 1; index < argc; ++index) {
        const std::string argument = argv[index];
        if (argument == "-h" || argument == "--help") {
            show_help = true;
            return true;
        }

        const bool known_option =
            argument == "--particles" || argument == "--repetitions" ||
            argument == "--warmup" || argument == "--output" ||
            argument == "--thickness" || argument == "--absorption" ||
            argument == "--scattering" || argument == "--seed" ||
            argument == "--max-collisions" || argument == "--cpu-label";
        if (!known_option) {
            error = "unknown option: " + argument;
            return false;
        }
        if (index + 1 >= argc) {
            error = argument + " requires a value";
            return false;
        }

        const std::string value = argv[++index];
        if (argument == "--particles") {
            if (!parse_particle_counts(value, options.particle_counts)) {
                error = "particle counts must be positive integers separated by commas";
                return false;
            }
        } else if (argument == "--repetitions") {
            if (!parse_unsigned(value, options.repetitions) ||
                options.repetitions == 0) {
                error = "repetitions must be a positive integer";
                return false;
            }
        } else if (argument == "--warmup") {
            if (!parse_unsigned(value, options.warmup_runs)) {
                error = "warm-up count must be a non-negative integer";
                return false;
            }
        } else if (argument == "--output") {
            if (value.empty()) {
                error = "output filename cannot be empty";
                return false;
            }
            options.output_filename = value;
        } else if (argument == "--thickness") {
            if (!parse_positive_double(
                    value, options.simulation.slab_thickness)) {
                error = "thickness must be a positive finite number";
                return false;
            }
        } else if (argument == "--absorption") {
            if (!parse_nonnegative_double(
                    value, options.material.absorption_coefficient)) {
                error = "absorption must be finite and non-negative";
                return false;
            }
        } else if (argument == "--scattering") {
            if (!parse_nonnegative_double(
                    value, options.material.scattering_coefficient)) {
                error = "scattering must be finite and non-negative";
                return false;
            }
        } else if (argument == "--seed") {
            if (!parse_unsigned(value, options.simulation.seed)) {
                error = "seed must be an unsigned integer";
                return false;
            }
        } else if (argument == "--max-collisions") {
            if (!parse_unsigned(
                    value,
                    options.simulation.max_collisions_per_particle) ||
                options.simulation.max_collisions_per_particle == 0) {
                error = "maximum collisions must be a positive integer";
                return false;
            }
        } else if (argument == "--cpu-label") {
            if (value.empty()) {
                error = "CPU label cannot be empty";
                return false;
            }
            options.cpu_label = value;
        }
    }

    return true;
}

std::string remove_matching_quotes(std::string value) {
    if (value.size() >= 2 &&
        ((value.front() == '"' && value.back() == '"') ||
         (value.front() == '\'' && value.back() == '\''))) {
        return value.substr(1, value.size() - 2);
    }
    return value;
}

std::string trim_whitespace(std::string value) {
    const std::size_t first = value.find_first_not_of(" \t\r");
    if (first == std::string::npos) {
        return {};
    }
    const std::size_t last = value.find_last_not_of(" \t\r");
    return value.substr(first, last - first + 1);
}

std::string value_from_file(
    const std::string& filename,
    const std::string& key,
    char separator) {
    std::ifstream file(filename);
    std::string line;
    while (std::getline(file, line)) {
        const std::size_t position = line.find(separator);
        if (position != std::string::npos &&
            trim_whitespace(line.substr(0, position)) == key) {
            return remove_matching_quotes(
                trim_whitespace(line.substr(position + 1)));
        }
    }
    return {};
}

Environment detect_environment(const BenchmarkOptions& options) {
    std::string cpu = options.cpu_label;
    std::string operating_system = MONTE_CARLO_SYSTEM_NAME;

#if defined(__linux__)
    if (cpu.empty()) {
        cpu = value_from_file("/proc/cpuinfo", "model name", ':');
    }
    const std::string pretty_name =
        value_from_file("/etc/os-release", "PRETTY_NAME", '=');
    if (!pretty_name.empty()) {
        operating_system = pretty_name;
    }
#endif

    if (cpu.empty()) {
        cpu = MONTE_CARLO_SYSTEM_PROCESSOR;
    }

    return {
        cpu,
        operating_system,
        MONTE_CARLO_SYSTEM_PROCESSOR,
        MONTE_CARLO_COMPILER_ID,
        MONTE_CARLO_COMPILER_VERSION,
        MONTE_CARLO_BUILD_TYPE,
        MONTE_CARLO_CXX_FLAGS,
    };
}

std::string csv_escape(const std::string& value) {
    if (value.find_first_of(",\"\n") == std::string::npos) {
        return value;
    }

    std::string escaped{"\""};
    for (const char character : value) {
        if (character == '"') {
            escaped += "\"\"";
        } else {
            escaped += character;
        }
    }
    escaped += '"';
    return escaped;
}

double median(std::vector<double> values) {
    std::sort(values.begin(), values.end());
    const std::size_t middle = values.size() / 2;
    if (values.size() % 2 == 0) {
        return (values[middle - 1] + values[middle]) / 2.0;
    }
    return values[middle];
}

double median_absolute_deviation(
    const std::vector<double>& values,
    double value_median) {
    std::vector<double> deviations;
    deviations.reserve(values.size());
    for (const double value : values) {
        deviations.push_back(std::abs(value - value_median));
    }
    return median(std::move(deviations));
}

TimedRun run_once(
    const BenchmarkOptions& options,
    std::uint64_t particle_count,
    std::uint64_t repetition) {
    auto config = options.simulation;
    config.particle_count = particle_count;

    const auto start = std::chrono::steady_clock::now();
    const auto results = transport::simulate(options.material, config);
    const double elapsed_milliseconds =
        std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - start)
            .count();

    const double particles_per_second =
        static_cast<double>(particle_count) /
        (elapsed_milliseconds / 1000.0);
    return {
        particle_count,
        repetition,
        elapsed_milliseconds,
        particles_per_second,
        results,
    };
}

bool write_raw_results(
    const BenchmarkOptions& options,
    const Environment& environment,
    const std::vector<TimedRun>& runs) {
    std::ofstream file(options.output_filename);
    if (!file) {
        std::cerr << "Error: could not open " << options.output_filename
                  << '\n';
        return false;
    }

    file << "particle_count,repetition,elapsed_ms,particles_per_second,"
            "seed,slab_thickness,absorption_coefficient,"
            "scattering_coefficient,max_collisions_per_particle,"
            "transmitted,reflected,absorbed,scattered_at_least_once,"
            "total_collisions,total_path_length,cpu,operating_system,"
            "architecture,compiler_id,compiler_version,build_type,"
            "compiler_flags,thread_count\n";
    file << std::setprecision(17);

    for (const auto& run : runs) {
        file << run.particle_count << ',' << run.repetition << ','
             << run.elapsed_milliseconds << ',' << run.particles_per_second
             << ',' << options.simulation.seed << ','
             << options.simulation.slab_thickness << ','
             << options.material.absorption_coefficient << ','
             << options.material.scattering_coefficient << ','
             << options.simulation.max_collisions_per_particle << ','
             << run.results.transmitted << ',' << run.results.reflected << ','
             << run.results.absorbed << ','
             << run.results.scattered_at_least_once << ','
             << run.results.total_collisions << ','
             << run.results.total_path_length << ','
             << csv_escape(environment.cpu) << ','
             << csv_escape(environment.operating_system) << ','
             << csv_escape(environment.architecture) << ','
             << csv_escape(environment.compiler_id) << ','
             << csv_escape(environment.compiler_version) << ','
             << csv_escape(environment.build_type) << ','
             << csv_escape(environment.compiler_flags) << ",1\n";
    }

    if (!file) {
        std::cerr << "Error: failed while writing "
                  << options.output_filename << '\n';
        return false;
    }
    return true;
}

int run_benchmarks(const BenchmarkOptions& options) {
    const Environment environment = detect_environment(options);
    std::vector<TimedRun> all_runs;
    all_runs.reserve(
        options.particle_counts.size() * options.repetitions);

    std::cout << "Serial Particle Transport Benchmark\n"
              << "===================================\n"
              << "Repetitions: " << options.repetitions
              << ", warm-ups: " << options.warmup_runs
              << ", seed: " << options.simulation.seed << '\n'
              << "CPU: " << environment.cpu << '\n'
              << "OS: " << environment.operating_system
              << " (" << environment.architecture << ")\n"
              << "Compiler: " << environment.compiler_id << ' '
              << environment.compiler_version << '\n'
              << "Build: " << environment.build_type
              << ", flags: " << environment.compiler_flags << '\n'
              << "Threads: 1 (serial baseline)\n\n"
              << std::left << std::setw(14) << "Particles"
              << std::right << std::setw(14) << "Median (ms)"
              << std::setw(14) << "MAD (ms)"
              << std::setw(20) << "Particles/second" << '\n';

    for (const std::uint64_t particle_count : options.particle_counts) {
        for (std::uint64_t warmup = 0;
             warmup < options.warmup_runs;
             ++warmup) {
            static_cast<void>(run_once(options, particle_count, warmup));
        }

        std::vector<double> elapsed_times;
        elapsed_times.reserve(options.repetitions);
        for (std::uint64_t repetition = 1;
             repetition <= options.repetitions;
             ++repetition) {
            auto run = run_once(options, particle_count, repetition);
            elapsed_times.push_back(run.elapsed_milliseconds);
            all_runs.push_back(std::move(run));
        }

        const double median_milliseconds = median(elapsed_times);
        const double deviation =
            median_absolute_deviation(elapsed_times, median_milliseconds);
        const double throughput =
            static_cast<double>(particle_count) /
            (median_milliseconds / 1000.0);

        std::cout << std::left << std::setw(14) << particle_count
                  << std::right << std::fixed << std::setprecision(3)
                  << std::setw(14) << median_milliseconds
                  << std::setw(14) << deviation
                  << std::setprecision(0) << std::setw(20) << throughput
                  << '\n';
    }

    if (!write_raw_results(options, environment, all_runs)) {
        return 1;
    }
    std::cout << "\nRaw results saved to " << options.output_filename << '\n';
    return 0;
}

}  // namespace

int main(int argc, char* argv[]) {
    BenchmarkOptions options;
    bool show_help = false;
    std::string error;

    if (!parse_options(argc, argv, options, show_help, error)) {
        std::cerr << "Error: " << error << "\n\n";
        print_usage(argv[0]);
        return 1;
    }
    if (show_help) {
        print_usage(argv[0]);
        return 0;
    }

    try {
        return run_benchmarks(options);
    } catch (const std::exception& exception) {
        std::cerr << "Benchmark failed: " << exception.what() << '\n';
        return 1;
    }
}
