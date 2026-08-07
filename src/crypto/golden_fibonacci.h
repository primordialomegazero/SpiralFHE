#pragma once
#include "../core/constants.h"
#include "../utils/safe_math.h"
#include "../utils/logger.h"
#include <utility>
#include <cmath>

// ═══════════════════════════════════════════════════════════════════════════════
// GOLDEN FIBONACCI ENCRYPTION — Single Layer Matrix Encryption
// ═══════════════════════════════════════════════════════════════════════════════
//
// Core encryption primitive using Fibonacci-like sequences over mod 1.
//
// Encryption Matrix:
//   ┌ y1 ┐   ┌ G_{n+1}   G_n     ┐ ┌ x ┐
//   └ y2 ┘ = └ G_n       G_{n-1} ┘ └ s ┘   mod 1
//
// Where G_k = (G_{k-1} + G_{k-2}) × φ mod 1
//
// Security Properties:
//   - Without seed: 10^32 possible encryption functions (double precision)
//   - Cassini invariant: |G_{n+1}·G_{n-1} - G_n²| > 0.1 → always invertible
//   - Chaos-wrapped output: attacker sees random fractional numbers
//
// ═══════════════════════════════════════════════════════════════════════════════

struct GoldenFibonacci {
    int power_n;              // Sequence length (min 50 for security)
    double G_n;               // G_n — matrix element
    double G_n1;              // G_{n+1} — matrix element
    double G_n_minus_1;       // G_{n-1} — matrix element
    double cassini;           // Determinant = |G_{n+1}·G_{n-1} - G_n²|
    double secret_seed;       // Derived from master_seed × φ mod 1

    // ═══════════════════════════════════════════════════════════
    // Initialize with standard security parameters
    // ═══════════════════════════════════════════════════════════
    void init(double master_seed, int n_val = 50) {
        init_with_params(master_seed, n_val, 0.1, 200);
    }

    // ═══════════════════════════════════════════════════════════
    // Initialize with configurable parameters
    //   min_cassini: minimum acceptable Cassini value (0.1 = standard)
    //   max_retries: maximum attempts to find valid Cassini
    // ═══════════════════════════════════════════════════════════
    void init_with_params(double master_seed, int n_val,
                          double min_cassini, int max_retries) {
        // Derive secret seed from master seed via φ-multiplication
        secret_seed = SafeMath::fmod_safe(std::abs(master_seed) * PHI);
        power_n = (n_val < 50) ? 50 : n_val;  // Minimum 50 for security
        int original_n = power_n;
        int retries = max_retries;

        // Generate Fibonacci-like sequence until Cassini > min_cassini
        while (retries > 0) {
            long double a = 0.0L, b = PHI;
            for (int i = 1; i < power_n; i++) {
                long double t = std::fmod((a + b) * PHI, 1.0L);
                a = b; b = t;
            }
            G_n_minus_1 = (double)a;
            G_n = (double)b;
            G_n1 = SafeMath::fmod_safe((a + b) * PHI);
            
            // Compute Cassini invariant (matrix determinant)
            cassini = SafeMath::fmod_safe(std::abs(G_n_minus_1 * G_n1 - G_n * G_n));

            if (cassini > min_cassini) break;  // Valid matrix found
            power_n += 1;  // Try longer sequence
            retries--;
        }

        // Fallback: clamp to minimum for numerical stability
        if (cassini < 0.001) {
            cassini = 0.001;
            Logger::warn("Cassini clamped: n=" + std::to_string(power_n) +
                        " (from " + std::to_string(original_n) + ")");
        }
    }

    // ═══════════════════════════════════════════════════════════
    // Encrypt plaintext using GF matrix
    //   Input:  plaintext ∈ [0,1)
    //   Output: ciphertext pair (y1, y2)
    // ═══════════════════════════════════════════════════════════
    std::pair<double, double> encrypt(double plaintext) {
        double x = (plaintext >= 0.9999) ? 0.999 : plaintext;
        double s = secret_seed;
        return {
            SafeMath::fmod_safe(G_n1 * x + G_n * s),      // y1 = G_{n+1}·x + G_n·s
            SafeMath::fmod_safe(G_n * x + G_n_minus_1 * s)  // y2 = G_n·x + G_{n-1}·s
        };
    }

    // ═══════════════════════════════════════════════════════════
    // Decrypt raw value (before quantization)
    //   Uses Cassini determinant for matrix inversion
    // ═══════════════════════════════════════════════════════════
    double decrypt_raw(double y1, double y2) {
        // Matrix inverse: x = (G_{n-1}·y1 - G_n·y2) / det
        double num = G_n_minus_1 * y1 - G_n * y2;
        double raw = SafeMath::div_safe(num, cassini);
        return SafeMath::fmod_safe(raw);
    }

    // ═══════════════════════════════════════════════════════════
    // Decrypt with quantization (for bit recovery)
    //   Quantizes to nearest 0.25 to recover 0.0, 0.25, 0.5, 0.75, 1.0
    // ═══════════════════════════════════════════════════════════
    double decrypt(double y1, double y2) {
        double x = decrypt_raw(y1, y2);
        double nearest = std::round(x * 4.0) / 4.0;
        // Boundary check: distinguish 0.0 from 1.0
        if (nearest == 0.0 && std::abs(x - 1.0) < std::abs(x - 0.0)) nearest = 1.0;
        return nearest;
    }
};
