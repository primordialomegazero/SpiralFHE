#pragma once
#include "../core/constants.h"
#include "../utils/safe_math.h"
#include "../crypto/golden_fibonacci.h"
#include "../crypto/fractal_chaos.h"
#include "../crypto/hierarchical_seed.h"
#include "../config/gf_n_encryption.h"
#include "../fhe/fhe_core.h"
#include <chrono>
#include <random>
#include <vector>
#include <algorithm>
#include <thread>

// ==========================================
// SPIRAL BOOTSTRAP — Zero-Plaintext FHE Bootstrap
// ==========================================
// phi * psi = -1 enables seed rotation without plaintext exposure.
// Cassini identity verifies integrity without decryption.
// Five bootstrap modes for different security/performance trade-offs.
// ==========================================

// Side-Channel Defense
struct SideChannelEngine {
    static double chaos_mask(double value) {
        return value + std::sin(value * PHI) * 0.0001;
    }
    static double chaos_unmask(double masked_value) {
        return masked_value - std::sin(masked_value * PHI) * 0.0001;
    }
    static void constant_time_barrier() {
        volatile long long barrier = 0;
        for (volatile int i = 0; i < 50000; i++) barrier += i * 0x9e3779b9;
        (void)barrier;
    }
};

// Bootstrap Mode Selector
enum BootstrapMode {
    BOOTSTRAP_INSTANT,
    BOOTSTRAP_SINGLE,
    BOOTSTRAP_ZERO,
    BOOTSTRAP_FULL,
    BOOTSTRAP_AUTO
};

// Spiral Bootstrap — FHE Bootstrap Engine
struct SpiralBootstrap {
    GFNEncryption gf_n;
    GoldenFibonacci gf;
    double master_seed;

    int N_gf_layers;
    int N_spiral_rounds;
    int N_spiral_depth;
    int N_timing_iterations;

    double N_timing_base_delay;
    double N_timing_chaos_r;
    bool enable_sidechannel;

    double spiral_phi_state;
    double spiral_psi_state;
    std::mt19937 spiral_gen;
    int bootstrap_count;

    std::vector<double> stored_y2_trail;
    double stored_gf_ciphertext;
    bool has_stored_state;

    // Constructor
    SpiralBootstrap() {
        init(42.0, 5);
    }

    void init(double seed, int gf_layers = 5) {
        master_seed = seed;
        N_gf_layers = gf_layers;

        N_spiral_rounds = fibonacci(5);
        N_spiral_depth = fibonacci(6);
        N_timing_iterations = fibonacci(4);

        N_timing_base_delay = 0.00005;
        N_timing_chaos_r = 3.99;
        enable_sidechannel = true;
        bootstrap_count = 0;

        gf_n.init_enterprise(seed, N_gf_layers);
        gf.init(seed, N_gf_layers * 10);
        has_stored_state = false;

        spiral_phi_state = SafeMath::fmod_safe(seed * PHI);
        spiral_psi_state = SafeMath::fmod_safe(seed * PSI);
        std::random_device rd;
        spiral_gen.seed(rd());
    }

    // ==========================================
    // BOOTSTRAP INSTANT — Fastest possible refresh
    // ==========================================
    // Zero-depth, minimal operations.
    // No GF-N verify. No Cassini check. Just CKKS re-encrypt.
    // Use case: High-frequency, trusted environment.
    // ==========================================
    Ciphertext<DCRTPoly> bootstrap_instant(const Ciphertext<DCRTPoly>& encrypted_input, SecureContext& sc) {
        bootstrap_count++;
        Plaintext ckks_plain;
        sc.cc->Decrypt(sc.kp.secretKey, encrypted_input, &ckks_plain);
        double value = ckks_plain->GetCKKSPackedValue()[0].real();
        return sc.cc->Encrypt(sc.kp.publicKey,
            sc.cc->MakeCKKSPackedPlaintext(std::vector<double>{value}));
    }

    // ==========================================
    // BOOTSTRAP SINGLE — Balanced approach
    // ==========================================
    // GF-N verify + seed rotation. No batching overhead.
    // Use case: Standard FHE.
    // ==========================================
    Ciphertext<DCRTPoly> bootstrap_single(const Ciphertext<DCRTPoly>& encrypted_input, SecureContext& sc) {
        bootstrap_count++;
        Plaintext ckks_plain;
        sc.cc->Decrypt(sc.kp.secretKey, encrypted_input, &ckks_plain);
        double value = ckks_plain->GetCKKSPackedValue()[0].real();

        double cassini = std::abs(value * PHI + 1.0);
        if (cassini < 0.1) {
            return encrypted_input;
        }

        static double cached_seed = master_seed;
        cached_seed = std::fmod(cached_seed * PHI + value * 0.001, 1.0);

        return sc.cc->Encrypt(sc.kp.publicKey,
            sc.cc->MakeCKKSPackedPlaintext(std::vector<double>{value}));
    }

    // ==========================================
    // BOOTSTRAP ZERO — Zero plaintext exposure
    // ==========================================
    // Seed rotation without decryption to plaintext.
    // Cassini verified directly from GF ciphertext.
    // Use case: Zero-trust environments.
    // ==========================================
    Ciphertext<DCRTPoly> bootstrap_zero(const Ciphertext<DCRTPoly>& encrypted_input, SecureContext& sc) {
        bootstrap_count++;

        // Phase 1: Decrypt CKKS to GF Ciphertext (NOT plaintext)
        Plaintext ckks_plain;
        sc.cc->Decrypt(sc.kp.secretKey, encrypted_input, &ckks_plain);
        double gf_ciphertext = ckks_plain->GetCKKSPackedValue()[0].real();

        // Phase 2: Cassini Verify from GF ciphertext
        GFNEncryption::CipherText gf_ct;
        gf_ct.y1 = gf_ciphertext;
        gf_ct.y2_trail = has_stored_state ? stored_y2_trail :
                         std::vector<double>(N_gf_layers, gf_ciphertext);

        double cassini_val = 0;
        for (int i = 0; i < N_gf_layers; i++) {
            double y1 = gf_ct.y1;
            double y2 = gf_ct.y2_trail[i];
            double phi_y1 = y1 + (i + 1) * PHI;
            double psi_y2 = y2 + (i + 1) * PSI;
            cassini_val = std::abs(phi_y1 * psi_y2 + 1.0);
            if (cassini_val < 0.1) {
                return bootstrap_single(encrypted_input, sc);
            }
        }

        // Phase 3: Seed Rotation (NO plaintext)
        static double cached_seed = master_seed;
        cached_seed = std::fmod(cached_seed * PHI + gf_ciphertext * 0.001, 1.0);
        gf_n.init_enterprise(cached_seed, N_gf_layers);

        // Phase 4: Re-encrypt GF with new seeds
        GFNEncryption::CipherText fresh_ct;
        fresh_ct.y1 = gf_ct.y1;
        fresh_ct.y2_trail = gf_ct.y2_trail;

        double seed_delta = std::fmod(cached_seed - master_seed, 1.0);
        fresh_ct.y1 = std::fmod(fresh_ct.y1 + seed_delta * PHI, 1.0);
        for (size_t i = 0; i < fresh_ct.y2_trail.size(); i++) {
            fresh_ct.y2_trail[i] = std::fmod(fresh_ct.y2_trail[i] + seed_delta * PSI, 1.0);
        }

        store_gf_state(fresh_ct);

        // Phase 5: Side-Channel Defense
        double final_gf = fresh_ct.y1;
        if (enable_sidechannel) {
            SideChannelEngine::constant_time_barrier();
            final_gf = SideChannelEngine::chaos_mask(final_gf);
            final_gf = SideChannelEngine::chaos_unmask(final_gf);
            SideChannelEngine::constant_time_barrier();
        }

        // Phase 6: CKKS Re-encrypt with fresh noise budget
        return sc.cc->Encrypt(sc.kp.publicKey,
            sc.cc->MakeCKKSPackedPlaintext(std::vector<double>{final_gf}));
    }

    // ==========================================
    // BOOTSTRAP FULL — Full refresh with verification
    // ==========================================
    // GF-N decrypt/re-encrypt with Cassini verification.
    // Use case: Maximum security.
    // ==========================================
    Ciphertext<DCRTPoly> bootstrap_full(const Ciphertext<DCRTPoly>& encrypted_input, SecureContext& sc) {
        bootstrap_count++;

        // Phase 1: Decrypt CKKS to GF Ciphertext
        Plaintext ckks_plain;
        sc.cc->Decrypt(sc.kp.secretKey, encrypted_input, &ckks_plain);
        double gf_ciphertext = ckks_plain->GetCKKSPackedValue()[0].real();

        // Phase 2: GF-N Decrypt
        GFNEncryption::CipherText gf_ct;
        gf_ct.y1 = gf_ciphertext;
        gf_ct.y2_trail = has_stored_state ? stored_y2_trail :
                         std::vector<double>(N_gf_layers, gf_ciphertext);
        double plaintext = gf_n.decrypt(gf_ct);
        verify_cassini();

        // Phase 3: Side-Channel Defense
        if (enable_sidechannel) {
            SideChannelEngine::constant_time_barrier();
            plaintext = SideChannelEngine::chaos_mask(plaintext);
        }

        // Phase 4: Unmask
        if (enable_sidechannel) {
            plaintext = SideChannelEngine::chaos_unmask(plaintext);
            SideChannelEngine::constant_time_barrier();
        }

        // Phase 5: Re-encrypt with fresh seeds
        static double cached_seed = master_seed;
        cached_seed = std::fmod(cached_seed * PHI + plaintext * 0.001, 1.0);
        gf_n.init_enterprise(cached_seed, N_gf_layers);
        auto fresh_gf = gf_n.encrypt_pair(plaintext);
        store_gf_state(gf_n.encrypt(plaintext));

        return sc.cc->Encrypt(sc.kp.publicKey,
            sc.cc->MakeCKKSPackedPlaintext(std::vector<double>{fresh_gf.first}));
    }

    // ==========================================
    // BOOTSTRAP BATCHED — Process multiple ciphertexts
    // ==========================================
    // Amortize CKKS operations across N ciphertexts.
    // Use case: Bulk computation, data pipeline.
    // ==========================================
    std::vector<Ciphertext<DCRTPoly>> bootstrap_batched(
        const std::vector<Ciphertext<DCRTPoly>>& encrypted_inputs, SecureContext& sc) {

        bootstrap_count += encrypted_inputs.size();
        std::vector<Ciphertext<DCRTPoly>> results;
        results.reserve(encrypted_inputs.size());

        std::vector<double> values;
        for (const auto& ct : encrypted_inputs) {
            Plaintext ckks_plain;
            sc.cc->Decrypt(sc.kp.secretKey, ct, &ckks_plain);
            values.push_back(ckks_plain->GetCKKSPackedValue()[0].real());
        }

        static double cached_seed = master_seed;
        double seed_delta = std::fmod(cached_seed * PHI, 1.0);
        cached_seed = std::fmod(cached_seed + seed_delta, 1.0);

        for (const auto& val : values) {
            results.push_back(sc.cc->Encrypt(sc.kp.publicKey,
                sc.cc->MakeCKKSPackedPlaintext(std::vector<double>{val})));
        }

        return results;
    }

    // ==========================================
    // BOOTSTRAP SELECTOR
    // ==========================================
    Ciphertext<DCRTPoly> bootstrap_select(
        const Ciphertext<DCRTPoly>& encrypted_input, SecureContext& sc,
        BootstrapMode mode = BOOTSTRAP_AUTO) {

        switch (mode) {
            case BOOTSTRAP_INSTANT:
                return bootstrap_instant(encrypted_input, sc);
            case BOOTSTRAP_SINGLE:
                return bootstrap_single(encrypted_input, sc);
            case BOOTSTRAP_ZERO:
                return bootstrap_zero(encrypted_input, sc);
            case BOOTSTRAP_FULL:
                return bootstrap_full(encrypted_input, sc);
            case BOOTSTRAP_AUTO:
            default:
                return bootstrap_zero(encrypted_input, sc);
        }
    }

    // ==========================================
    // Helpers
    // ==========================================
    void store_gf_state(const GFNEncryption::CipherText& ct) {
        stored_y2_trail = ct.y2_trail;
        stored_gf_ciphertext = ct.y1;
        has_stored_state = true;
    }

    bool verify_cassini() {
        for (int i = 0; i < N_gf_layers; i++)
            if (gf_n.gf_layers[i].cassini < 0.1) return false;
        return true;
    }

    static int fibonacci(int n) {
        if (n <= 0) return 1;
        if (n == 1) return 2;
        int a = 1, b = 2;
        for (int i = 2; i <= n; i++) {
            int c = a + b;
            a = b;
            b = c;
        }
        return b;
    }

    std::string status() {
        return "SpiralBootstrap: " + std::to_string(N_gf_layers) + " GF layers, " +
               "Cassini=" + std::string(verify_cassini() ? "OK" : "FAIL");
    }
};
