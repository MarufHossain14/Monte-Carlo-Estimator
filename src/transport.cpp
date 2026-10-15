#include "transport/transport.hpp"

#include <cmath>
#include <random>
#include <stdexcept>

namespace transport {
namespace {

void validate(const Material& material, const SimulationConfig& config) {
    const auto is_nonnegative_finite = [](double value) {
        return std::isfinite(value) && value >= 0.0;
    };

    if (!is_nonnegative_finite(material.absorption_coefficient)) {
        throw std::invalid_argument(
            "absorption coefficient must be finite and non-negative");
    }
    if (!is_nonnegative_finite(material.scattering_coefficient)) {
        throw std::invalid_argument(
            "scattering coefficient must be finite and non-negative");
    }
    if (!std::isfinite(config.slab_thickness) ||
        config.slab_thickness <= 0.0) {
        throw std::invalid_argument("slab thickness must be finite and positive");
    }
    if (config.particle_count == 0) {
        throw std::invalid_argument("particle count must be positive");
    }
    if (config.max_collisions_per_particle == 0) {
        throw std::invalid_argument(
            "maximum collisions per particle must be positive");
    }

    // Checking the sum separately also catches overflow from two individually
    // finite coefficients.
    if (!std::isfinite(material.total_interaction_coefficient())) {
        throw std::invalid_argument(
            "total interaction coefficient must be finite");
    }
}

struct ParticleResult {
    Outcome outcome;
    std::uint64_t collisions;
    double path_length;
    bool scattered;
};

ParticleResult simulate_particle(
    const Material& material,
    const SimulationConfig& config,
    std::mt19937_64& engine) {
    const double total_coefficient = material.total_interaction_coefficient();

    // A vacuum has no interaction sites. Handling it explicitly also avoids
    // constructing an exponential distribution with a zero rate.
    if (total_coefficient == 0.0) {
        return {Outcome::transmitted, 0, config.slab_thickness, false};
    }

    std::exponential_distribution<double> free_path(total_coefficient);
    std::bernoulli_distribution is_absorbed(
        material.absorption_coefficient / total_coefficient);
    std::bernoulli_distribution points_right(0.5);

    double position = 0.0;
    double path_length = 0.0;
    int direction = 1;
    std::uint64_t collisions = 0;
    bool scattered = false;

    while (collisions < config.max_collisions_per_particle) {
        const double distance_to_boundary =
            direction > 0 ? config.slab_thickness - position : position;
        const double sampled_free_path = free_path(engine);

        if (sampled_free_path >= distance_to_boundary) {
            path_length += distance_to_boundary;
            const Outcome outcome = direction > 0
                ? Outcome::transmitted
                : Outcome::reflected;
            return {outcome, collisions, path_length, scattered};
        }

        position += direction * sampled_free_path;
        path_length += sampled_free_path;
        ++collisions;

        if (is_absorbed(engine)) {
            return {Outcome::absorbed, collisions, path_length, scattered};
        }

        // "Isotropic" scattering in this one-dimensional model means that
        // both possible travel directions are equally likely.
        direction = points_right(engine) ? 1 : -1;
        scattered = true;
    }

    throw std::runtime_error(
        "particle exceeded the configured maximum collision count");
}

void add_to_results(
    SimulationResults& results,
    const ParticleResult& particle) {
    switch (particle.outcome) {
        case Outcome::transmitted:
            ++results.transmitted;
            break;
        case Outcome::reflected:
            ++results.reflected;
            break;
        case Outcome::absorbed:
            ++results.absorbed;
            break;
    }

    if (particle.scattered) {
        ++results.scattered_at_least_once;
    }
    results.total_collisions += particle.collisions;
    results.total_path_length += particle.path_length;
}

}  // namespace

double Material::total_interaction_coefficient() const noexcept {
    return absorption_coefficient + scattering_coefficient;
}

std::uint64_t SimulationResults::completed_particles() const noexcept {
    return transmitted + reflected + absorbed;
}

SimulationResults simulate(
    const Material& material,
    const SimulationConfig& config) {
    validate(material, config);

    std::mt19937_64 engine(config.seed);
    SimulationResults results;

    for (std::uint64_t particle = 0; particle < config.particle_count; ++particle) {
        add_to_results(results, simulate_particle(material, config, engine));
    }

    return results;
}

}  // namespace transport
