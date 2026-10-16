#include "transport/transport.hpp"

#include <charconv>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

struct Options {
    transport::Material material{0.5, 0.5};
    transport::SimulationConfig simulation{};
};

void print_usage(const char* program_name) {
    std::cout
        << "Usage: " << program_name << " [options]\n\n"
        << "Simulate particles entering a one-dimensional material slab.\n\n"
        << "Options:\n"
        << "  -n, --particles <count>   Number of particles (default: 100000)\n"
        << "  --thickness <value>       Slab thickness (default: 1.0)\n"
        << "  --absorption <value>      Absorption coefficient (default: 0.5)\n"
        << "  --scattering <value>      Scattering coefficient (default: 0.5)\n"
        << "  --seed <number>           Random seed (default: 5489)\n"
        << "  --max-collisions <count>  Per-particle safety limit (default: 1000000)\n"
        << "  -h, --help                Show this help message\n";
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

bool parse_double(const std::string& text, double& value) {
    std::size_t parsed_characters = 0;

    try {
        value = std::stod(text, &parsed_characters);
    } catch (const std::invalid_argument&) {
        return false;
    } catch (const std::out_of_range&) {
        return false;
    }

    return parsed_characters == text.size() && std::isfinite(value);
}

bool read_value(
    int argc,
    char* argv[],
    int& index,
    const std::string& option,
    std::string& value,
    std::string& error) {
    if (index + 1 >= argc) {
        error = option + " requires a value";
        return false;
    }

    value = argv[++index];
    return true;
}

bool parse_options(
    int argc,
    char* argv[],
    Options& options,
    bool& show_help,
    std::string& error) {
    for (int index = 1; index < argc; ++index) {
        const std::string argument = argv[index];

        if (argument == "-h" || argument == "--help") {
            show_help = true;
            return true;
        }

        const bool known_option =
            argument == "-n" || argument == "--particles" ||
            argument == "--thickness" || argument == "--absorption" ||
            argument == "--scattering" || argument == "--seed" ||
            argument == "--max-collisions";
        if (!known_option) {
            error = "unknown option: " + argument;
            return false;
        }

        std::string value;
        if (!read_value(argc, argv, index, argument, value, error)) {
            return false;
        }

        if (argument == "-n" || argument == "--particles") {
            if (!parse_unsigned(value, options.simulation.particle_count) ||
                options.simulation.particle_count == 0) {
                error = "particle count must be a positive integer";
                return false;
            }
        } else if (argument == "--thickness") {
            if (!parse_double(value, options.simulation.slab_thickness) ||
                options.simulation.slab_thickness <= 0.0) {
                error = "thickness must be a positive finite number";
                return false;
            }
        } else if (argument == "--absorption") {
            if (!parse_double(value, options.material.absorption_coefficient) ||
                options.material.absorption_coefficient < 0.0) {
                error =
                    "absorption coefficient must be finite and non-negative";
                return false;
            }
        } else if (argument == "--scattering") {
            if (!parse_double(value, options.material.scattering_coefficient) ||
                options.material.scattering_coefficient < 0.0) {
                error =
                    "scattering coefficient must be finite and non-negative";
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
        }
    }

    return true;
}

void print_outcome(
    const std::string& label,
    std::uint64_t count,
    std::uint64_t total) {
    const double probability =
        static_cast<double>(count) / static_cast<double>(total);
    std::cout << "  " << std::left << std::setw(13) << label << std::right
              << std::setw(10) << count << "  "
              << std::fixed << std::setprecision(4)
              << probability * 100.0 << "%\n";
}

void print_results(
    const Options& options,
    const transport::SimulationResults& results,
    std::chrono::duration<double, std::milli> elapsed) {
    const double particle_count =
        static_cast<double>(options.simulation.particle_count);

    std::cout << "Particle Transport Simulation\n"
              << "=============================\n"
              << "Particles:              "
              << options.simulation.particle_count << '\n'
              << "Slab thickness:         "
              << options.simulation.slab_thickness << '\n'
              << "Absorption coefficient: "
              << options.material.absorption_coefficient << '\n'
              << "Scattering coefficient: "
              << options.material.scattering_coefficient << '\n'
              << "Random seed:            " << options.simulation.seed << "\n\n"
              << "Outcomes:\n";

    print_outcome(
        "Transmitted", results.transmitted, options.simulation.particle_count);
    print_outcome(
        "Reflected", results.reflected, options.simulation.particle_count);
    print_outcome(
        "Absorbed", results.absorbed, options.simulation.particle_count);

    std::cout << "\nAdditional totals:\n"
              << "  Scattered particles:  "
              << results.scattered_at_least_once << '\n'
              << "  Total collisions:     " << results.total_collisions << '\n'
              << std::fixed << std::setprecision(4)
              << "  Mean collisions:      "
              << static_cast<double>(results.total_collisions) /
                     particle_count
              << '\n'
              << "  Mean path length:     "
              << results.total_path_length / particle_count << '\n'
              << std::setprecision(3)
              << "  Simulation time:      " << elapsed.count() << " ms\n";
}

}  // namespace

int main(int argc, char* argv[]) {
    Options options;
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
        const auto start = std::chrono::steady_clock::now();
        const auto results =
            transport::simulate(options.material, options.simulation);
        const auto elapsed = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - start);

        print_results(options, results, elapsed);
    } catch (const std::exception& exception) {
        std::cerr << "Simulation failed: " << exception.what() << '\n';
        return 1;
    }

    return 0;
}
