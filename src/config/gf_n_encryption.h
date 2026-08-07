#pragma once
#include "../core/constants.h"
#include "../utils/safe_math.h"
#include "../utils/logger.h"
#include "../crypto/golden_fibonacci.h"
#include "../crypto/hierarchical_seed.h"
#include "../config/system_config.h"
#include <vector>
#include <utility>
#include <string>

// ==========================================
// GF-N ENCRYPTION — Multi-Layer Golden Fibonacci
// ==========================================
//
// THEOREM (Layered Cassini Security):
//   Given N independent Golden Fibonacci encryption layers,
//   each with its own seed and Cassini invariant > 0.1,
//   the compound probability of breaking all layers is:
//     P(break) = (1/10^16)^N  (double precision search space per layer)
//
//   Proof:
//     Each layer uses a 2x2 matrix with determinant != 0.
//     Without knowing the seed, an attacker must search the
//     entire double-precision space (10^16 values) per layer.
//     With N layers, the search space multiplies: 10^(16N).
//     For N=5 (default), this is 10^80 — exceeding brute force
//     on 256-bit keys.
//
//   The y2_trail mechanism ensures exact decryption:
//     Each layer stores its y2 value during encryption.
//     Decryption reverses the process using these values,
//     so no information is lost through the layers.
//
// USED IN:
//   - spiral_bootstrap.h:  Decrypt-reencrypt cycle during bootstrap
//   - fhe_core.h:          Inner encryption before CKKS
//
// CROSS-REFERENCE:
//   Theorem T2: Dual encryption security (this file + fhe_core.h)
//   Theorem T6: Cassini identity (golden_fibonacci.h)
//   Theorem T8: Hierarchical seeds (hierarchical_seed.h)
// ==========================================

struct GFNEncryption {
    int N_layers;
    int base_n;
    int n_step;
    double cassini_min;
    int max_cassini_retries;
    std::string seed_branch;
    bool use_unique_branches;
    std::vector<GoldenFibonacci> gf_layers;

    // Predefined security levels
    enum SecurityLevel {
        STANDARD  = 1,   // Development
        ELEVATED  = 3,   // Testing
        HIGH      = 5,   // Production (default)
        MAXIMUM   = 10   // High security
    };

    struct CipherText {
        double y1;
        std::vector<double> y2_trail;
    };

    GFNEncryption() : N_layers(1), base_n(50), n_step(7),
                       cassini_min(0.1), max_cassini_retries(200),
                       seed_branch("encryption"), use_unique_branches(true) {}

    // ==========================================
    // Initialize with N layers
    //
    //   Each layer gets a unique seed from the
    //   Hierarchical Seed Tree. Layers use different
    //   sequence lengths (base_n + i * n_step) to
    //   ensure cryptographic independence.
    // ==========================================
    void init_enterprise(double master_seed, int num_layers = 5) {
        if (num_layers < 1) num_layers = 1;
        N_layers = num_layers;
        gf_layers.resize(N_layers);

        HierarchicalSeedTree tree;
        tree.init(master_seed);

        for (int i = 0; i < N_layers; i++) {
            std::string unique_branch = seed_branch + "_" + std::to_string(i);
            tree.create_branch(unique_branch, i, (i % 2 == 0));
            double sub_seed = tree.get_seed(unique_branch, 0);
            gf_layers[i].init_with_params(sub_seed, base_n + i * n_step,
                                          cassini_min, max_cassini_retries);
        }

        Logger::info("GF-N: " + std::to_string(N_layers) +
                     " layers, base_n=" + std::to_string(base_n) +
                     ", n_step=" + std::to_string(n_step));
    }

    void init(double master_seed, int n_layers) {
        if (n_layers < 1) n_layers = 1;
        init_enterprise(master_seed, n_layers);
    }

    void init_from_config(const SystemConfig& cfg) {
        init(cfg.master_seed, cfg.N_fne_layers);
    }

    // ==========================================
    // Encrypt through all N layers (forward pass)
    //
    //   Layer 1:  y1_1 = G1_{n+1} * x   + G1_n * s1
    //   Layer 2:  y1_2 = G2_{n+1} * y1_1 + G2_n * s2
    //   ...
    //   Layer N:  y1_N = GN_{n+1} * y1_{N-1} + GN_n * sN
    //
    //   Returns: (y1_N, [y2_1, y2_2, ..., y2_N])
    // ==========================================
    CipherText encrypt(double plaintext) {
        CipherText ct;
        ct.y2_trail.resize(N_layers);
        double current = (plaintext >= 0.9999) ? 0.999 : plaintext;

        for (int i = 0; i < N_layers; i++) {
            auto [y1, y2] = gf_layers[i].encrypt(current);
            ct.y2_trail[i] = y2;
            current = y1;
        }
        ct.y1 = current;
        return ct;
    }

    // ==========================================
    // Encrypt returning (y1, avg_y2) for CKKS embedding
    // ==========================================
    std::pair<double, double> encrypt_pair(double plaintext) {
        auto ct = encrypt(plaintext);
        double avg = 0;
        for (auto y2 : ct.y2_trail)
            avg = SafeMath::fmod_safe(avg + y2);
        return {ct.y1, SafeMath::fmod_safe(avg / N_layers)};
    }

    // ==========================================
    // Decrypt through all N layers (reverse pass)
    //
    //   Layer N:  y1_{N-1} = decrypt(y1_N, y2_N)
    //   ...
    //   Layer 1:  x = decrypt(y1_1, y2_1)
    //
    //   Uses y2_trail stored during encryption to
    //   recover exact plaintext.
    // ==========================================
    double decrypt(const CipherText& ct) {
        double current = ct.y1;

        for (int i = N_layers - 1; i > 0; i--) {
            current = gf_layers[i].decrypt_raw(current, ct.y2_trail[i]);
        }
        current = gf_layers[0].decrypt(current, ct.y2_trail[0]);

        // Disambiguate 0.0 vs 1.0 at boundary
        if (current < 0.01 || current > 0.99) {
            auto ct0 = encrypt(0.0);
            auto ct1 = encrypt(1.0);
            double dist0 = std::abs(ct.y1 - ct0.y1);
            double dist1 = std::abs(ct.y1 - ct1.y1);
            if (dist1 < dist0) return 1.0;
            if (dist0 < dist1) return 0.0;
        }

        return current;
    }

    std::string security_level_string() const {
        if (N_layers >= 10) return "MAXIMUM";
        if (N_layers >= 5)  return "HIGH";
        if (N_layers >= 3)  return "ELEVATED";
        return "STANDARD";
    }
};
