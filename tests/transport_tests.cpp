#include "transport/transport.hpp"

#include <cmath>
#include <cstdint>
#include <functional>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

namespace {

void expect(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

void expect_invalid_argument(
    const std::function<void()>& operation,
    const std::string& message) {
    try {
        operation();
    } catch (const std::invalid_argument&) {
        return;
    }
    throw std::runtime_error(message);
}

void expect_runtime_error(
    const std::function<void()>& operation,
    const std::string& message) {
    try {
        operation();
    } catch (const std::runtime_error&) {
        return;
    }
    throw std::runtime_error(message);
}

bool same_results(
    const transport::SimulationResults& left,
    const transport::SimulationResults& right) {
    return left.transmitted == right.transmitted &&
        left.reflected == right.reflected &&
        left.absorbed == right.absorbed &&
        left.scattered_at_least_once == right.scattered_at_least_once &&
        left.total_collisions == right.total_collisions &&
        left.total_path_length == right.total_path_length;
}

void expect_near(
    double actual,
    double expected,
    double tolerance,
    const std::string& message) {
    if (std::abs(actual - expected) > tolerance) {
        throw std::runtime_error(message);
    }
}

void test_vacuum_transmits_every_particle() {
    const transport::Material vacuum{};
    const transport::SimulationConfig config{1'000, 2.5, 42, 100};

    const auto results = transport::simulate(vacuum, config);

    expect(results.transmitted == config.particle_count,
           "vacuum should transmit every particle");
    expect(results.reflected == 0, "vacuum should not reflect particles");
    expect(results.absorbed == 0, "vacuum should not absorb particles");
    expect(results.total_collisions == 0,
           "vacuum should not produce collisions");
    expect(results.scattered_at_least_once == 0,
           "vacuum should not scatter particles");
    expect(results.total_path_length ==
               config.slab_thickness * config.particle_count,
           "vacuum path length should equal slab thickness per particle");
}

void test_outcomes_conserve_particle_count() {
    const transport::Material material{0.7, 1.3};
    const transport::SimulationConfig config{20'000, 3.0, 987654321, 100'000};

    const auto results = transport::simulate(material, config);

    expect(results.completed_particles() == config.particle_count,
           "terminal outcome counts should conserve particle count");
    expect(results.scattered_at_least_once <= config.particle_count,
           "scattered-particle count cannot exceed particle count");
    expect(results.total_collisions >= results.absorbed,
           "every absorbed particle must have at least one collision");
}

void test_fixed_seed_is_reproducible() {
    const transport::Material material{0.4, 1.6};
    const transport::SimulationConfig config{5'000, 1.5, 12345, 100'000};

    const auto first = transport::simulate(material, config);
    const auto second = transport::simulate(material, config);

    expect(same_results(first, second),
           "the same configuration and seed should reproduce results");
}

void test_absorption_only_matches_analytical_solution() {
    const transport::Material absorber{0.8, 0.0};
    const transport::SimulationConfig config{200'000, 1.5, 77, 10};

    const auto results = transport::simulate(absorber, config);

    expect(results.reflected == 0,
           "particles cannot reflect without scattering");
    expect(results.scattered_at_least_once == 0,
           "absorption-only material cannot scatter particles");
    expect(results.transmitted + results.absorbed == config.particle_count,
           "absorption-only outcomes should conserve particle count");
    expect(results.total_collisions == results.absorbed,
           "each absorbed particle should have exactly one collision");

    // With no scattering, a particle is transmitted only when its first
    // interaction distance exceeds the slab thickness. The exact probability
    // is exp(-Sigma_a * L), providing an independent mathematical reference.
    const double expected_transmission = std::exp(
        -absorber.absorption_coefficient * config.slab_thickness);
    const double observed_transmission =
        static_cast<double>(results.transmitted) /
        static_cast<double>(config.particle_count);
    const double expected_standard_error = std::sqrt(
        expected_transmission * (1.0 - expected_transmission) /
        static_cast<double>(config.particle_count));

    expect_near(
        observed_transmission,
        expected_transmission,
        6.0 * expected_standard_error,
        "absorption-only transmission should match exp(-Sigma_a * L)");
}

void test_scattering_only_never_absorbs() {
    const transport::Material scatterer{0.0, 1.5};
    const transport::SimulationConfig config{20'000, 2.0, 2468, 100'000};

    const auto results = transport::simulate(scatterer, config);

    expect(results.absorbed == 0,
           "zero absorption coefficient should prevent absorption");
    expect(results.transmitted + results.reflected == config.particle_count,
           "scattering-only exit counts should conserve particle count");
    expect(results.scattered_at_least_once > 0,
           "an interacting scattering material should scatter particles");
    expect(results.total_collisions >= results.scattered_at_least_once,
           "each scattered particle should have at least one collision");
}

void test_invalid_configuration_is_rejected() {
    const transport::Material valid_material{0.5, 0.5};
    transport::SimulationConfig config{};

    config.particle_count = 0;
    expect_invalid_argument(
        [&] { static_cast<void>(transport::simulate(valid_material, config)); },
        "zero particles should be rejected");

    config = {};
    config.slab_thickness = 0.0;
    expect_invalid_argument(
        [&] { static_cast<void>(transport::simulate(valid_material, config)); },
        "zero slab thickness should be rejected");

    config.slab_thickness = -1.0;
    expect_invalid_argument(
        [&] { static_cast<void>(transport::simulate(valid_material, config)); },
        "negative slab thickness should be rejected");

    config.slab_thickness = std::numeric_limits<double>::infinity();
    expect_invalid_argument(
        [&] { static_cast<void>(transport::simulate(valid_material, config)); },
        "infinite slab thickness should be rejected");

    config.slab_thickness = std::numeric_limits<double>::quiet_NaN();
    expect_invalid_argument(
        [&] { static_cast<void>(transport::simulate(valid_material, config)); },
        "NaN slab thickness should be rejected");

    config = {};
    config.max_collisions_per_particle = 0;
    expect_invalid_argument(
        [&] { static_cast<void>(transport::simulate(valid_material, config)); },
        "a zero collision limit should be rejected");

    config = {};
    const transport::Material negative_absorption{-0.1, 0.5};
    expect_invalid_argument(
        [&] { static_cast<void>(transport::simulate(negative_absorption, config)); },
        "negative coefficients should be rejected");

    const transport::Material negative_scattering{0.5, -0.1};
    expect_invalid_argument(
        [&] { static_cast<void>(transport::simulate(negative_scattering, config)); },
        "negative scattering should be rejected");

    const transport::Material infinite_scattering{
        0.5, std::numeric_limits<double>::infinity()};
    expect_invalid_argument(
        [&] { static_cast<void>(transport::simulate(infinite_scattering, config)); },
        "infinite coefficients should be rejected");

    const transport::Material nan_absorption{
        std::numeric_limits<double>::quiet_NaN(), 0.5};
    expect_invalid_argument(
        [&] { static_cast<void>(transport::simulate(nan_absorption, config)); },
        "NaN coefficients should be rejected");

    const double largest = std::numeric_limits<double>::max();
    const transport::Material overflowing_total{largest, largest};
    expect_invalid_argument(
        [&] { static_cast<void>(transport::simulate(overflowing_total, config)); },
        "an overflowing total interaction coefficient should be rejected");
}

void test_collision_limit_stops_pathological_history() {
    const transport::Material scatterer{0.0, 1.0};
    const transport::SimulationConfig config{1, 1e100, 42, 1};

    expect_runtime_error(
        [&] { static_cast<void>(transport::simulate(scatterer, config)); },
        "a history exceeding the collision guard should fail clearly");
}

void test_probability_estimates_include_uncertainty() {
    const auto half = transport::estimate_probability(50, 100);
    expect_near(half.probability, 0.5, 1e-12,
                "probability should equal count divided by total");
    expect_near(half.standard_error, 0.05, 1e-12,
                "binomial standard error should be calculated correctly");
    expect(half.confidence_lower_95 < half.probability &&
               half.confidence_upper_95 > half.probability,
           "confidence interval should contain the observed probability");

    const auto zero = transport::estimate_probability(0, 100);
    expect_near(zero.confidence_lower_95, 0.0, 1e-15,
                "zero observed outcomes should have a zero lower bound");
    expect(zero.confidence_upper_95 > 0.0,
           "zero observed outcomes should retain a nonzero upper bound");

    expect_invalid_argument(
        [] { static_cast<void>(transport::estimate_probability(0, 0)); },
        "a zero probability total should be rejected");
    expect_invalid_argument(
        [] { static_cast<void>(transport::estimate_probability(11, 10)); },
        "an outcome count above the total should be rejected");
}

void test_many_seeds_follow_expected_uncertainty() {
    constexpr std::uint64_t trial_count = 24;
    constexpr std::uint64_t particles_per_trial = 20'000;
    const transport::Material absorber{0.8, 0.0};
    transport::SimulationConfig config{
        particles_per_trial, 1.5, 0, 10};

    const double expected_probability = std::exp(
        -absorber.absorption_coefficient * config.slab_thickness);
    const double trial_standard_error = std::sqrt(
        expected_probability * (1.0 - expected_probability) /
        static_cast<double>(particles_per_trial));

    double probability_sum = 0.0;
    double squared_standardized_error_sum = 0.0;
    std::uint64_t trials_within_95_percent = 0;

    for (std::uint64_t trial = 0; trial < trial_count; ++trial) {
        config.seed = 10'000 + trial;
        const auto results = transport::simulate(absorber, config);
        const double observed_probability =
            static_cast<double>(results.transmitted) /
            static_cast<double>(particles_per_trial);
        const double standardized_error =
            (observed_probability - expected_probability) /
            trial_standard_error;

        probability_sum += observed_probability;
        squared_standardized_error_sum +=
            standardized_error * standardized_error;
        if (std::abs(standardized_error) <= 1.96) {
            ++trials_within_95_percent;
        }
    }

    const double mean_probability =
        probability_sum / static_cast<double>(trial_count);
    const double mean_standard_error =
        trial_standard_error / std::sqrt(static_cast<double>(trial_count));
    expect_near(
        mean_probability,
        expected_probability,
        6.0 * mean_standard_error,
        "mean transmission across seeds should match the analytical value");

    // Standardized errors should have an RMS near one when the predicted
    // binomial uncertainty describes the run-to-run variation correctly.
    const double standardized_error_rms = std::sqrt(
        squared_standardized_error_sum / static_cast<double>(trial_count));
    expect(standardized_error_rms > 0.4 && standardized_error_rms < 1.8,
           "run-to-run variation should match predicted sampling uncertainty");
    expect(trials_within_95_percent >= 14,
           "most independent trials should fall within two standard errors");
}

}  // namespace

int main() {
    try {
        test_vacuum_transmits_every_particle();
        test_outcomes_conserve_particle_count();
        test_fixed_seed_is_reproducible();
        test_absorption_only_matches_analytical_solution();
        test_scattering_only_never_absorbs();
        test_invalid_configuration_is_rejected();
        test_collision_limit_stops_pathological_history();
        test_probability_estimates_include_uncertainty();
        test_many_seeds_follow_expected_uncertainty();
    } catch (const std::exception& error) {
        std::cerr << "FAILED: " << error.what() << '\n';
        return 1;
    }

    std::cout << "All particle transport tests passed.\n";
    return 0;
}
