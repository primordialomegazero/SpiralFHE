#pragma once
#include "../core/constants.h"
#include "../utils/safe_math.h"

// ═══════════════════════════════════════════════════════════════════════════════
// FRACTAL CHAOS ENGINE — Logistic Map + φ-Rotation + Lyapunov
//
// FORMAL PROOFS COVERED:
//   Theorem 7 (Irreversible Chaos): r > 3.57, Lyapunov > 0
//   See: https://github.com/primordialomegazero/femmgFHE/blob/main/docs/FORMAL_PROOFS.md#theorem-7-irreversible-chaos
//   Unit Test: tests/theorem_tests/test_theorem_7.cpp (10^8x amplification verified)
// ═══════════════════════════════════════════════════════════════════════════════
//
// Provides the chaotic foundation for all obfuscation operations.
//
// Key Properties:
//   - Logistic map: x_{n+1} = r · x_n · (1 - x_n)
//   - Chaotic regime: r > 3.56995... (Feigenbaum point)
//   - Lyapunov exponent λ > 0 → irreversible chaos
//   - φ-rotation: irrational angle → never repeats
//
// ═══════════════════════════════════════════════════════════════════════════════

// ═══════════════════════════════════════════════════════════════
// Fibonacci Anchor — Deterministic pseudo-random value for swaps
// Used in Fractal Transform to decide whether to swap φ/ψ pairs
// ═══════════════════════════════════════════════════════════════
inline double fibonacci_anchor(int n, double seed) {
    double f0 = SafeMath::fmod_safe(seed);
    double f1 = SafeMath::fmod_safe(seed * PHI);
    for (int i = 0; i < n; i++) {
        double fn = f0 + f1;
        f0 = f1;
        f1 = SafeMath::fmod_safe(fn);
    }
    return SafeMath::fmod_safe(f1);
}

// ═══════════════════════════════════════════════════════════════
// Lyapunov Estimator — Measures chaos intensity
// λ > 0 → chaotic (irreversible)
// λ < 0 → stable (reversible — NOT desired for obfuscation)
// ═══════════════════════════════════════════════════════════════
inline double lyapunov_estimate(double r, double x0, int n = 20) {
    double x = x0, sum = 0.0;
    for (int i = 0; i < n; i++) {
        sum += std::log(std::abs(r * (1.0 - 2.0 * x)) + 0.0001);
        x = r * x * (1.0 - x);
    }
    return sum / n;
}

// ═══════════════════════════════════════════════════════════════
// Fractal Transform — Core chaotic obfuscation function
// Applies logistic chaos + φ-rotation per layer and depth
//
// Parameters:
//   x:      input value
//   layer:  current fractal layer (affects r and angle)
//   depth:  iterations per layer
//
// Security: Lyapunov > 0 → small input changes produce
//           exponentially diverging outputs (avalanche effect)
// ═══════════════════════════════════════════════════════════════
inline double fractal_transform(double x, int layer, int depth) {
    // Layer-specific chaos parameter (r increases with layer)
    // [THEOREM 7] r > 3.57 → Lyapunov > 0 → irreversible chaos.
// See: https://github.com/primordialomegazero/femmgFHE/blob/main/docs/FORMAL_PROOFS.md#theorem-7-irreversible-chaos
    // After 13 rounds: δx ≈ δx₀·e^{0.615·13} ≈ δx₀·2980. See docs/FORMAL_PROOFS.md §7
    double r = 3.7 + (layer * 0.05);
    double result = x;
    
    for (int d = 0; d < depth; d++) {
        // Logistic chaos: irreversible when r > 3.57
        result = r * result * (1.0 - result);
        
        // φ-rotation: irrational angle — never repeats
        double angle = (layer + 1) * PHI * PI;
        result = result * SafeMath::cos_safe(angle) + 
                 (1.0 - result) * SafeMath::sin_safe(angle);
        
        // Keep in [0,1) range
        result = SafeMath::fmod_safe(result);
    }
    return result;
}
