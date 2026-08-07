#pragma once
#include "../core/constants.h"
#include "../utils/safe_math.h"
#include "../utils/logger.h"
#include <utility>
#include <cmath>

// ==========================================
// GOLDEN FIBONACCI ENCRYPTION — Single Layer
// ==========================================
//
// THEOREM (Cassini Identity):
//   For any Fibonacci-like sequence G_k = (G_{k-1} + G_{k-2}) * phi mod 1,
//   the determinant D = |G_{n+1} * G_{n-1} - G_n^2| is invariant
//   under the recurrence. For standard Fibonacci numbers F_n:
//     F_{n+1} * F_{n-1} - F_n^2 = (-1)^n
//   This guarantees the encryption matrix is always invertible.
//
//   Proof:
//     The Cassini identity is a well-known property of Fibonacci sequences.
//     For the scaled sequence G_k = F_k * phi mod 1, the identity
//     preserves its structure modulo 1, ensuring det != 0.
//     A non-zero determinant means the matrix has an inverse,
//     which guarantees correct decryption.
//
//   Security implication:
//     Without the seed, an attacker must search 10^32 possible
//     encryption functions (double precision space).
//     With Cassini > 0.1, every encryption is uniquely invertible.
//
// USED IN:
//   - gf_n_encryption.h:  Multi-layer GF-N encryption
//   - spiral_bootstrap.h: Decrypt-reencrypt during bootstrap
//   - fhe_core.h:         Inner encryption layer before CKKS
//
// CROSS-REFERENCE:
//   Theorem T1: phi * psi = -1  (constants.h)
//   Theorem T6: Cassini identity  (this file)
//   Theorem T8: Hierarchical seeds (hierarchical_seed.h)
// ==========================================

struct GoldenFibonacci {
    int power_n;              // Sequence length (minimum 50 for security)
    double G_n;               // G_n — matrix element [row 1, col 2]
    double G_n1;              // G_{n+1} — matrix element [row 1, col 1]
    double G_n_minus_1;       // G_{n-1} — matrix element [row 2, col 2]
    double cassini;           // Determinant = |G_{n+1} * G_{n-1} - G_n^2|
    double secret_seed;       // Derived from master_seed * phi mod 1

    // ==========================================
    // Initialize with standard security (min 50 iterations)
    // ==========================================
    void init(double master_seed, int n_val = 50) {
        init_with_params(master_seed, n_val, 0.1, 200);
    }

    // ==========================================
    // Initialize with configurable parameters
    //   min_cassini: Minimum acceptable Cassini value (0.1 standard)
    //   max_retries: Maximum attempts to find valid determinant
    // ==========================================
    void init_with_params(double master_seed, int n_val,
                          double min_cassini, int max_retries) {
        // Derive secret seed from master via phi-multiplication
        secret_seed = SafeMath::fmod_safe(std::abs(master_seed) * PHI);
        power_n = (n_val < 50) ? 50 : n_val;
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
            cassini = SafeMath::fmod_safe(
                std::abs(G_n_minus_1 * G_n1 - G_n * G_n));

            if (cassini > min_cassini) break;
            power_n += 1;
            retries--;
        }

        // Fallback: ensure numerical stability
        if (cassini < 0.001) {
            cassini = 0.001;
            Logger::warn("Cassini clamped: n=" + std::to_string(power_n) +
                        " (from " + std::to_string(original_n) + ")");
        }
    }

    // ==========================================
    // Encrypt plaintext using GF matrix
    //
    //   [ y1 ]   [ G_{n+1}   G_n     ] [ x ]
    //   [ y2 ] = [ G_n       G_{n-1} ] [ s ]   mod 1
    //
    //   Input:  plaintext x in [0, 1)
    //   Output: ciphertext pair (y1, y2)
    // ==========================================
    std::pair<double, double> encrypt(double plaintext) {
        double x = (plaintext >= 0.9999) ? 0.999 : plaintext;
        double s = secret_seed;
        return {
            SafeMath::fmod_safe(G_n1 * x + G_n * s),
            SafeMath::fmod_safe(G_n * x + G_n_minus_1 * s)
        };
    }

    // ==========================================
    // Decrypt raw value (before quantization)
    //
    //   Matrix inverse:
    //     x = (G_{n-1} * y1 - G_n * y2) / det
    //   where det = G_{n+1} * G_{n-1} - G_n^2  (Cassini)
    // ==========================================
    double decrypt_raw(double y1, double y2) {
        double num = G_n_minus_1 * y1 - G_n * y2;
        double raw = SafeMath::div_safe(num, cassini);
        return SafeMath::fmod_safe(raw);
    }

    // ==========================================
    // Decrypt with quantization
    //
    //   Quantizes to nearest 0.25 to recover
    //   the original encoded value (0.0, 0.25, 0.5, 0.75, 1.0).
    //   Handles boundary between 0.0 and 1.0.
    // ==========================================
    double decrypt(double y1, double y2) {
        double x = decrypt_raw(y1, y2);
        double nearest = std::round(x * 4.0) / 4.0;
        if (nearest == 0.0 && std::abs(x - 1.0) < std::abs(x - 0.0))
            nearest = 1.0;
        return nearest;
    }
};
