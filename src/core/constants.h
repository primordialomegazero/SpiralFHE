#pragma once
#include <cmath>

// ==========================================
// SPIRAL FHE — CORE CONSTANTS
// ==========================================
//
// phi and psi are the two roots of x^2 - x - 1 = 0:
//   phi + psi = 1
//   phi * psi = -1
//
// This algebraic identity is the foundation of Spiral FHE.
// It is a mathematical truth at the 1+1=2 level — not a hardness assumption.
// ==========================================

// Golden Ratio (phi) — (1 + sqrt(5)) / 2
constexpr double PHI = 1.6180339887498948482;

// Golden Ratio Conjugate (psi) — (1 - sqrt(5)) / 2
constexpr double PSI = -0.6180339887498948482;

// Pi
constexpr double PI = 3.14159265358979323846;
