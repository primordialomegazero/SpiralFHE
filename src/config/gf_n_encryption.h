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

// ═══════════════════════════════════════════════════════════════════════════════
// GF-N ENCRYPTION — N-Layer Golden Fibonacci Encryption
// ═══════════════════════════════════════════════════════════════════════════════
//
// Stacks N independent Golden Fibonacci encryption layers.
// Each layer uses a unique seed from the Hierarchical Seed Tree.
//
// Security Levels:
//   STANDARD:  N=1   (development)
//   ELEVATED:  N=3   (testing)
//   ENTERPRISE: N=5  (production)
//   MILITARY:  N=10  (high security)
//   UNIVERSE:  N=100 (maximum)
//
// Key Properties:
//   - Each layer: unique seed, unique Cassini invariant
//   - y2_trail: breadcrumb trail for exact decryption
//   - Compound security: (1/10^16)^N probability of breaking all layers
//
// ═══════════════════════════════════════════════════════════════════════════════

struct GFNEncryption {
    int N_layers;                          // Number of GF layers (configurable)
    int base_n;                            // Base n for GF (default 50)
    int n_step;                            // n increment per layer (default 7)
    double cassini_min;                    // Minimum Cassini threshold (0.1)
    int max_cassini_retries;               // Max retries for stable n
    std::string seed_branch;               // Branch name for seeds
    bool use_unique_branches;              // Unique branch per layer
    std::vector<GoldenFibonacci> gf_layers; // The N GF encryption layers

    enum SecurityLevel {
        STANDARD = 1,
        ELEVATED = 3,
        MILITARY = 10,
        UNIVERSE = 100
    };

    struct CipherText {
        double y1;                          // Final ciphertext
        std::vector<double> y2_trail;       // y2 values per layer (for decryption)
    };

    GFNEncryption() : N_layers(1), base_n(50), n_step(7),
                       cassini_min(0.1), max_cassini_retries(200),
                       seed_branch("encryption"), use_unique_branches(true) {}

    // ═══════════════════════════════════════════════════════════
    // Enterprise initialization with unique branch per layer
    // ═══════════════════════════════════════════════════════════
    void init_enterprise(double master_seed, int num_sub_seeds = 1) {
        if (num_sub_seeds < 1) num_sub_seeds = 1;
        N_layers = num_sub_seeds;
        gf_layers.resize(N_layers);

        HierarchicalSeedTree tree;
        tree.init(master_seed);

        // Create unique branch per layer for cryptographic isolation
        for (int i = 0; i < N_layers; i++) {
            std::string unique_branch = seed_branch + "_" + std::to_string(i);
            tree.create_branch(unique_branch, i, (i % 2 == 0));
            double sub_seed = tree.get_seed(unique_branch, 0);
            gf_layers[i].init_with_params(sub_seed, base_n + i * n_step,
                                          cassini_min, max_cassini_retries);
        }

        Logger::info("Enterprise FNE: " + std::to_string(N_layers) +
                     " layers | branch=" + seed_branch +
                     " unique=" + (use_unique_branches ? "yes" : "no") +
                     " base_n=" + std::to_string(base_n) +
                     " n_step=" + std::to_string(n_step));
    }

    void init(double master_seed, int n_layers) {
        if (n_layers < 1) n_layers = 1;
        init_enterprise(master_seed, n_layers);
    }

    void init_from_config(const SystemConfig& cfg) { 
        init(cfg.master_seed, cfg.N_fne_layers); 
    }

    // ═══════════════════════════════════════════════════════════
    // Encrypt through all N layers
    // ═══════════════════════════════════════════════════════════
    CipherText encrypt(double plaintext) {
        CipherText ct;
        ct.y2_trail.resize(N_layers);
        double current = (plaintext >= 0.9999) ? 0.999 : plaintext;

        // Forward pass: Layer 1 → Layer N
        for (int i = 0; i < N_layers; i++) {
            auto [y1, y2] = gf_layers[i].encrypt(current);
            ct.y2_trail[i] = y2;  // Store y2 for decryption
            current = y1;          // y1 becomes input to next layer
        }
        ct.y1 = current;
        return ct;
    }

    // ═══════════════════════════════════════════════════════════
    // Encrypt returning (y1, avg_y2) pair
    // ═══════════════════════════════════════════════════════════
    std::pair<double, double> encrypt_pair(double plaintext) {
        auto ct = encrypt(plaintext);
        double avg = 0;
        for (auto y2 : ct.y2_trail) avg = SafeMath::fmod_safe(avg + y2);
        return {ct.y1, SafeMath::fmod_safe(avg / N_layers)};
    }

    // ═══════════════════════════════════════════════════════════
    // Decrypt through all N layers (reverse order)
    // ═══════════════════════════════════════════════════════════
    double decrypt(const CipherText& ct) {
        double current = ct.y1;

        // Reverse pass: Layer N → Layer 1
        for (int i = N_layers - 1; i > 0; i--) {
            current = gf_layers[i].decrypt_raw(current, ct.y2_trail[i]);
        }
        current = gf_layers[0].decrypt(current, ct.y2_trail[0]);

        // Boundary fix for 0.0 vs 1.0 disambiguation
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
        if (N_layers >= 100) return "UNIVERSE";
        if (N_layers >= 10)  return "MILITARY";
        if (N_layers >= 3)   return "ELEVATED";
        return "STANDARD";
    }
};
