#ifndef MONTE_CARLO_TRANSPORT_TRANSPORT_HPP
#define MONTE_CARLO_TRANSPORT_TRANSPORT_HPP

#include <cstdint>

namespace transport {

/** The mutually exclusive way in which a particle history ends. */
enum class Outcome {
    transmitted,
    reflected,
    absorbed,
};

/**
 * Macroscopic interaction coefficients for a homogeneous material.
 *
 * Coefficients have units of inverse length. Their sum is the rate parameter
 * of the exponential distribution used to sample a particle's free path.
 */
struct Material {
    double absorption_coefficient{0.0};
    double scattering_coefficient{0.0};

    [[nodiscard]] double total_interaction_coefficient() const noexcept;
};

/** Inputs controlling one complete simulation run. */
struct SimulationConfig {
    std::uint64_t particle_count{100'000};
    double slab_thickness{1.0};
    std::uint64_t seed{5489U};

    // This guard detects a pathological history or an incorrectly configured
    // model instead of allowing one particle to loop without a bound.
    std::uint64_t max_collisions_per_particle{1'000'000};
};

/** Aggregate tallies returned by the simulation core. */
struct SimulationResults {
    std::uint64_t transmitted{0};
    std::uint64_t reflected{0};
    std::uint64_t absorbed{0};
    std::uint64_t scattered_at_least_once{0};
    std::uint64_t total_collisions{0};
    double total_path_length{0.0};

    [[nodiscard]] std::uint64_t completed_particles() const noexcept;
};

/** Estimated binomial probability and its sampling uncertainty. */
struct ProbabilityEstimate {
    double probability;
    double standard_error;
    double confidence_lower_95;
    double confidence_upper_95;
};

/**
 * Calculate an observed outcome probability, its standard error, and a 95%
 * Wilson score interval. Wilson intervals remain useful when a count is near
 * zero or the total, where a simple normal interval can collapse.
 */
[[nodiscard]] ProbabilityEstimate estimate_probability(
    std::uint64_t outcome_count,
    std::uint64_t total_count);

/**
 * Simulate independent particles entering the left side of a one-dimensional
 * slab while initially travelling to the right.
 *
 * Invalid inputs throw std::invalid_argument. If a particle exceeds the
 * configured collision guard, the function throws std::runtime_error rather
 * than silently assigning an incorrect physical outcome.
 */
[[nodiscard]] SimulationResults simulate(
    const Material& material,
    const SimulationConfig& config);

}  // namespace transport

#endif
