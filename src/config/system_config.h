#pragma once
#include "../core/constants.h"
#include "../utils/safe_math.h"
#include <string>
#include <sstream>
#include <iomanip>

// ═══════════════════════════════════════════════════════════════════════════════
// SYSTEM CONFIG — 45 N-Configurable Parameters with 4 Preset Modes
// ═══════════════════════════════════════════════════════════════════════════════
//
// The central configuration hub for the entire Spiral Fractal iO system.
// Every parameter is N-configurable. Every default is φ-anchored.
//
// Preset modes provide sensible defaults:
//   DEV:         Fast, low security (RingDim 2048)
//   TEST:        Balanced (RingDim 4096)
//   PROD:        Full security (RingDim 32768)
//   ENTERPRISE:  Maximum security (RingDim 65536)
//
// All parameters can be individually overridden after setting a mode.
// The master_seed is auto-derived from all N values using φ-weighted hashing.
//
// ═══════════════════════════════════════════════════════════════════════════════

struct SystemConfig {
    // ═══════════════════════════════════════════════════════════
    // FHE PARAMETERS
    // ═══════════════════════════════════════════════════════════
    int N_ring_dim = 4096;           // CKKS ring dimension (powers of 2)
    int N_ckks_depth = 120;          // Multiplicative depth
    int N_batch_size = 256;          // CKKS batch size

    // ═══════════════════════════════════════════════════════════
    // ENCRYPTION PARAMETERS
    // ═══════════════════════════════════════════════════════════
    int N_fne_layers = 5;            // GF-N encryption layers
    int N_golden_base_n = 50;        // Base sequence length
    int N_golden_n_step = 7;         // Sequence increment per layer
    double N_cassini_min = 0.1;      // Minimum Cassini invariant
    int N_cassini_retries = 200;     // Max retries for stable Cassini

    // ═══════════════════════════════════════════════════════════
    // FRACTAL iO PARAMETERS
    // ═══════════════════════════════════════════════════════════
    int N_variants = 7;              // Fibonacci circuit variants
    int N_fractal_layers = 23;       // iO Refresh chaos layers
    int N_fractal_depth = 7;         // Chaos depth per layer
    double N_chaos_base = 3.7;       // Logistic chaos base r
    double N_chaos_step = 0.05;      // r increment per layer
    double N_chaos_r_max = 4.2;      // Maximum chaos parameter
    int N_chaos_iterations = 7;      // Iterations per FractalGate
    double N_refresh_weight = 0.618; // Blend weight (φ/(φ+1))

    // ═══════════════════════════════════════════════════════════
    // VALIDATION PARAMETERS
    // ═══════════════════════════════════════════════════════════
    int N_stats_samples = 100;       // KS test samples per pair
    int N_report_interval = 10;      // Progress report frequency
    int N_snapshot_interval = 10;    // KS snapshot frequency
    double N_ks_threshold = 0.05;    // Pass/fail threshold

    // ═══════════════════════════════════════════════════════════
    // AUTONOMOUS PARAMETERS
    // ═══════════════════════════════════════════════════════════
    int N_gates_target = 300;        // Target gates for chain
    int N_batch_size_auto = 50;      // Starting batch size
    int N_batch_min = 5;             // Minimum batch size
    int N_batch_max = 500;           // Maximum batch size
    double N_noise_warning = 0.55;   // Noise warning threshold
    double N_noise_critical = 0.85;  // Noise critical threshold
    double N_lyapunov_warning = 0.15;
    double N_lyapunov_critical = 0.05;
    int N_alert_cooldown = 5;        // Alert cooldown (seconds)
    int N_history_window = 100;      // Stability history size
    int N_prediction_min_samples = 10;

    // ═══════════════════════════════════════════════════════════
    // PERSISTENCE PARAMETERS
    // ═══════════════════════════════════════════════════════════
    int N_l1_entries = 10;           // FractalDB L1 rolling size
    int N_l2_hours = 24;             // L2 retention (hours)
    int N_checkpoint_interval = 50;  // Gates between checkpoints
    int N_archive_interval = 150;    // Gates between archives

    // ═══════════════════════════════════════════════════════════
    // DERIVED VALUES (auto-computed)
    // ═══════════════════════════════════════════════════════════
    double master_seed;              // Auto-derived from all N's
    int N_pairs;                     // variants choose 2
    int N_total_gates_variants;      // Sum of all Fibonacci gates

    // ═══════════════════════════════════════════════════════════
    // PRESET MODES
    // ═══════════════════════════════════════════════════════════
    void set_dev_mode() {
        N_ring_dim = 2048; N_ckks_depth = 60; N_fne_layers = 1;
        N_fractal_layers = 5; N_fractal_depth = 3;
        N_variants = 3; N_stats_samples = 10;
        N_gates_target = 50; N_batch_size_auto = 10;
        derive_all();
    }
    void set_test_mode() {
        N_ring_dim = 4096; N_ckks_depth = 120; N_fne_layers = 3;
        N_fractal_layers = 13; N_fractal_depth = 5;
        N_variants = 5; N_stats_samples = 50;
        N_gates_target = 150; N_batch_size_auto = 30;
        derive_all();
    }
    void set_prod_mode() {
        N_ring_dim = 32768; N_ckks_depth = 200; N_fne_layers = 5;
        N_fractal_layers = 23; N_fractal_depth = 7;
        N_variants = 12; N_stats_samples = 100;
        N_gates_target = 300; N_batch_size_auto = 50;
        derive_all();
    }
    void set_enterprise_mode() {
        N_ring_dim = 65536; N_ckks_depth = 300; N_fne_layers = 13;
        N_fractal_layers = 34; N_fractal_depth = 10;
        N_variants = 23; N_stats_samples = 1000;
        N_gates_target = 1000; N_batch_size_auto = 100;
        derive_all();
    }

    // ═══════════════════════════════════════════════════════════
    // Auto-derive master_seed and dependent values from all N's
    // ═══════════════════════════════════════════════════════════
    void derive_all() {
        master_seed = SafeMath::fmod_safe(
            N_variants * PHI + N_fractal_layers * PSI + 
            N_fractal_depth * PHI * PSI + N_fne_layers * PI +
            N_ring_dim * 1e-4 + N_ckks_depth * 1e-6);
        N_pairs = N_variants * (N_variants - 1) / 2;
        N_total_gates_variants = 0;
        for (int i = 0; i < N_variants; i++)
            N_total_gates_variants += fibonacci(i);
    }

    static int fibonacci(int n) {
        if (n <= 0) return 1; if (n == 1) return 2;
        int a = 1, b = 2;
        for (int i = 2; i <= n; i++) { int c = a + b; a = b; b = c; }
        return b;
    }

    std::string mode_string() {
        if (N_variants <= 3 && N_stats_samples <= 20) return "DEV";
        if (N_variants <= 5 && N_stats_samples <= 50) return "TEST";
        if (N_variants <= 12 && N_stats_samples <= 100) return "PROD";
        return "ENTERPRISE";
    }
};
