#pragma once
#include <cmath>

// ═══════════════════════════════════════════════════════════════════════════════
// SPIRAL FRACTAL iO — CORE CONSTANTS
// ═══════════════════════════════════════════════════════════════════════════════
//
// These constants are the mathematical foundation of the entire system.
// φ and ψ are the two roots of Y² - Y - 1 = 0, satisfying:
//   φ + ψ = 1
//   φ · ψ = -1
// This duality enables the DualGate projection system.
//
// ═══════════════════════════════════════════════════════════════════════════════

// Golden Ratio (φ) — The active computation root
// Value: (1 + √5) / 2 ≈ 1.6180339887498948482
// Used in: DualGate φ-projection, GF-N encryption, Fractal Transform
constexpr double PHI = 1.6180339887498948482;

// Golden Ratio Conjugate (ψ) — The passive reflection root
// Value: (1 - √5) / 2 ≈ -0.6180339887498948482
// Used in: DualGate ψ-projection, Spiral obfuscation
constexpr double PSI = -0.6180339887498948482;

// Feigenbaum Constant — Universal bifurcation rate in chaotic systems
// Used in: Emergent timing, Fractal chaos initialization
constexpr double FEIGENBAUM = 4.669201609102990;

// Omega Constant — Solution to x·e^x = 1
// Used in: Lambert W function approximations
constexpr double OMEGA = 0.5671432904097838;

// Pi — Circle constant
// Used in: φ-rotation angles, trigonometric chaos
constexpr double PI = 3.14159265358979323846;

// ═══════════════════════════════════════════════════════════════
// FRACTAL GOLDEN iO CONSTANTS
// ═══════════════════════════════════════════════════════════════

// Minimum stable depth for Fractal Golden iO (KS=0 at depth ≥ 3)
constexpr int IO_MIN_DEPTH = 3;

// Default obfuscation rounds (Fibonacci F(5) = 8)
constexpr int IO_DEFAULT_ROUNDS = 5;

// KS critical value threshold for indistinguishability
constexpr double IO_KS_CRITICAL = 0.05;

// iO Modes
enum class IOMode { STRUCTURAL, BLACKHOLE, OFF };
