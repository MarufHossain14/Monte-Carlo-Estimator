# Simplified Particle Transport Model

## Purpose

The model follows independent particles through a one-dimensional slab of
homogeneous material. It is intentionally small enough to validate before
profiling and optimizing its implementation.

## Assumptions

- The slab occupies positions from `0` to `L`, where `L > 0`.
- Every particle begins at position `0`, travelling into the slab.
- The material is homogeneous, so its interaction coefficients do not depend
  on position.
- Particles do not interact with one another.
- A particle keeps the same energy throughout its history.
- Scattering in one dimension selects the left or right direction with equal
  probability.
- Absorption ends a particle history immediately.

These assumptions make this an educational transport model, not a complete
physical description of radiation moving through matter.

## Sampling a history

The material has an absorption coefficient `Sigma_a` and a scattering
coefficient `Sigma_s`. Both must be finite and non-negative. Their sum is the
total interaction coefficient:

```text
Sigma_t = Sigma_a + Sigma_s
```

When `Sigma_t` is positive, the distance `d` to the next interaction follows
an exponential distribution:

```text
d = -log(U) / Sigma_t
```

where `U` is uniformly distributed between zero and one. If the sampled
interaction lies beyond the nearest slab boundary, the particle exits. It is
classified as transmitted when it exits to the right and reflected when it
exits to the left.

If the interaction occurs inside the slab, it is absorption with probability:

```text
P(absorption) = Sigma_a / Sigma_t
```

Otherwise it is scattering, and a new direction is sampled. A material with
`Sigma_t = 0` is a vacuum, so every particle crosses the slab without a
collision.

## Recorded results

The three terminal outcomes are mutually exclusive:

- transmitted;
- reflected;
- absorbed.

Their counts must add up to the number of simulated particles. The simulation
also records total collisions, total path length, and how many particles
scattered at least once. Scattering is not a terminal outcome because a
scattered particle can later be transmitted, reflected, or absorbed.

## Safety limit

A configurable collision limit guards against pathological histories and
implementation errors. Reaching it raises an error; the simulation never
silently converts that condition into a physical outcome.
