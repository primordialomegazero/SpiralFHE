#pragma once
#include <cmath>

// ==========================================
// SPIRAL FHE — CORE CONSTANTS
// ==========================================
//
// THEOREM (Golden Ratio Identity):
//   Let phi = (1+sqrt(5))/2 and psi = (1-sqrt(5))/2.
//   Then phi and psi are the two roots of x^2 - x - 1 = 0.
//   Therefore:
//     phi + psi = 1
//     phi * psi = -1
//
//   Proof:
//     phi * psi = ((1+sqrt(5))/2) * ((1-sqrt(5))/2)
//               = (1 - 5) / 4
//               = -4 / 4
//               = -1
//     phi + psi = ((1+sqrt(5)) + (1-sqrt(5))) / 2
//               = 2 / 2
//               = 1
//
//   This is not a conjecture or hardness assumption.
//   It is an algebraic identity provable from the definition.
//   No computational advance — classical or quantum — can alter these values.
//
// USED IN:
//   - golden_fibonacci.h:  Cassini invariant for encryption integrity
//   - spiral_bootstrap.h:  Seed rotation for zero-plaintext bootstrap
//   - fhe_core.h:          CKKS parameter derivation
//   - gf_n_encryption.h:   Multi-layer GF-N encryption
//
// CROSS-REFERENCE:
//   Theorem T1: phi * psi = -1  (this file)
//   Theorem T6: Cassini identity (golden_fibonacci.h)
//   Theorem T3: Zero-plaintext bootstrap (spiral_bootstrap.h)
// ==========================================

// phi — Golden Ratio
// Definition: (1 + sqrt(5)) / 2
// Property: phi * psi = -1, phi + psi = 1
constexpr double PHI = 1.6180339887498948482;

// psi — Golden Ratio Conjugate
// Definition: (1 - sqrt(5)) / 2
// Property: phi * psi = -1, phi + psi = 1
constexpr double PSI = -0.6180339887498948482;

// pi — Circle constant
// Definition: ratio of circumference to diameter
// Used in trigonometric chaos masking for side-channel defense
constexpr double PI = 3.14159265358979323846;
