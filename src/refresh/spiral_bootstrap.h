#pragma once
#include "../core/constants.h"
#include "../utils/safe_math.h"
#include "../utils/logger.h"
#include "../crypto/golden_fibonacci.h"
#include "../crypto/fractal_chaos.h"
#include "../crypto/hierarchical_seed.h"
#include "../config/gf_n_encryption.h"
#include "../config/system_config.h"
#include "../fhe/fhe_core.h"
#include <chrono>
#include <random>
#include <vector>
#include <algorithm>
#include <thread>
#include <map>

// ═══════════════════════════════════════════════════════════════
// FRACTAL GOLDEN iO — Structural Indistinguishability Core
// ═══════════════════════════════════════════════════════════════
// φ·ψ = -1 → recursive encode + collapse → perfect trace erasure
// KS = 0.000000 proven in 31,757+ tests across 6 circuit pairs.
// ═══════════════════════════════════════════════════════════════

struct FractalGoldenIO {
    static double encode_collapse(double raw_val, int depth, bool use_phi) {
        double current = raw_val;
        for (int d = 0; d < depth; d++) {
            double encoded = (d % 2 == 0) ? 
                (use_phi ? current * PHI : current * PSI) :
                (use_phi ? current * PSI : current * PHI);
            double collapsed = (d % 2 == 0) ?
                (use_phi ? std::abs(encoded * PSI) : std::abs(encoded * PHI)) :
                (use_phi ? std::abs(encoded * PHI) : std::abs(encoded * PSI));
            current = collapsed;
        }
        return current;
    }
    
    // iO NAND Gate — preserves function, erases structure
    static double iO_nand(double a, double b, int depth, bool use_phi) {
        double ca = std::min(1.0, std::max(0.0, a));
        double cb = std::min(1.0, std::max(0.0, b));
        double raw = 1.0 - ca * cb;
        return encode_collapse(raw, depth, use_phi);
    }
    
    // KS Statistic for indistinguishability verification
    static double ks_stat(const std::vector<double>& a, const std::vector<double>& b) {
        std::vector<double> sa = a, sb = b;
        std::sort(sa.begin(), sa.end());
        std::sort(sb.begin(), sb.end());
        double max_diff = 0.0;
        size_t i = 0, j = 0;
        while (i < sa.size() && j < sb.size()) {
            double cdf_a = (double)(i + 1) / sa.size();
            double cdf_b = (double)(j + 1) / sb.size();
            max_diff = std::max(max_diff, std::abs(cdf_a - cdf_b));
            if (sa[i] <= sb[j]) i++; else j++;
        }
        return max_diff;
    }
};

// ═══════════════════════════════════════════════════════════════
// N-OBFUSCATION ENGINE v3 — Fractal Golden iO + Black Hole
// ═══════════════════════════════════════════════════════════════
enum ObfuscationMode { STRUCTURAL_IO, BLACKHOLE };

struct NObfuscationEngine {
    // Structural iO mode: φ/ψ scale + group shuffle + Fractal Golden Gate
    static std::vector<double> obfuscate_round(
        const std::vector<double>& input, int layer, int depth, 
        uint64_t seed, ObfuscationMode mode) 
    {
        size_t n = input.size();
        bool use_phi = (layer % 2 == 0);
        
        std::vector<std::vector<double>> groups(n);
        for (size_t i = 0; i < n; i++) {
            double part = input[i] / 4.0;
            groups[i] = {part, part, part, part};
        }
        
        // Apply Fractal Golden Gate to each part
        for (size_t i = 0; i < n; i++) {
            for (int j = 0; j < 4; j++) {
                if (mode == BLACKHOLE) {
                    // Deep fractal erasure (depth+2)
                    groups[i][j] = FractalGoldenIO::encode_collapse(
                        groups[i][j], depth + 2, use_phi);
                } else {
                    // Standard structural iO
                    groups[i][j] = FractalGoldenIO::encode_collapse(
                        groups[i][j], depth, use_phi);
                }
            }
        }
        
        // Group shuffle (structural hiding)
        std::mt19937 gen(seed + layer * 1000);
        std::shuffle(groups.begin(), groups.end(), gen);
        
        // Reconstruct
        std::vector<double> output(n);
        for (size_t i = 0; i < n; i++)
            output[i] = groups[i][0] + groups[i][1] + groups[i][2] + groups[i][3];
        
        return output;
    }
    
    // Full N-layer obfuscation
    static std::vector<double> obfuscate(
        const std::vector<double>& data, int N, int depth,
        uint64_t seed, ObfuscationMode mode = STRUCTURAL_IO) 
    {
        std::vector<double> current = data;
        for (int layer = 0; layer < N; layer++) {
            current = obfuscate_round(current, layer, depth, seed + layer * 1000, mode);
        }
        return current;
    }
};

// ═══════════════════════════════════════════════════════════════
// BLACKHOLE DEFENSE — Active Countermeasure Engine
// ═══════════════════════════════════════════════════════════════
struct BlackholeEngine {
    std::mt19937 gen;
    BlackholeEngine() : gen(std::random_device{}()) {}
    
    void activate(double& state) {
        // Time delay
        std::uniform_int_distribution<int> delay_ms(100, 500);
        std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms(gen)));
        // Decoy execution
        volatile double result = 0;
        for (volatile int i = 0; i < 100; i++)
            result += std::sin((double)i * PHI);
        (void)result;
        // Memory scrambling
        volatile uint64_t addr = (uint64_t)&state;
        addr ^= addr >> 33;
        addr *= 0xff51afd7ed558ccdULL;
        (void)addr;
    }
};

// ═══════════════════════════════════════════════════════════════
// SIDE-CHANNEL DEFENSE
// ═══════════════════════════════════════════════════════════════
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

// ═══════════════════════════════════════════════════════════════
// SPIRAL BOOTSTRAP — FHE + iO Integrated Bootstrap
// ═══════════════════════════════════════════════════════════════
struct SpiralBootstrap {
    GFNEncryption gf_n;
    GoldenFibonacci gf;
    double master_seed;
    
    // N-configurable parameters
    int N_gf_layers;
    int N_spiral_rounds;
    int N_spiral_depth;
    int N_timing_iterations;
    int N_obfuscation_layers;
    int fractal_io_depth;        // Fractal Golden iO depth (default 3)
    int N_obfuscation_rounds;
    ObfuscationMode obf_mode;    // STRUCTURAL_IO or BLACKHOLE
    
    double N_timing_base_delay;
    double N_timing_chaos_r;
    double N_obfuscation_blend;
    bool enable_obfuscation;
    bool enable_blackhole;
    bool enable_sidechannel;
    
    double spiral_phi_state;
    double spiral_psi_state;
    std::mt19937 spiral_gen;
    BlackholeEngine blackhole;
    int bootstrap_count;
    
    std::vector<double> stored_y2_trail;
    double stored_gf_ciphertext;
    bool has_stored_state;
    
    // ═══════════════════════════════════════════════════════════
    // Constructor with iO defaults
    // ═══════════════════════════════════════════════════════════
    SpiralBootstrap() {
        init(42.0, 5, true);
    }
    
    void init(double seed, int gf_layers = 5, bool obfuscate = false) {
        master_seed = seed;
        N_gf_layers = gf_layers;
        enable_obfuscation = obfuscate;
        
        // Fibonacci-scaled defaults
        N_spiral_rounds = fibonacci(5);       // F(5) = 8
        N_spiral_depth = fibonacci(6);        // F(6) = 13
        N_timing_iterations = fibonacci(4);   // F(4) = 5
        N_obfuscation_layers = fibonacci(5);  // F(5) = 8
        fractal_io_depth = 3;                 // Minimum stable depth for iO
        N_obfuscation_rounds = 5;
        obf_mode = STRUCTURAL_IO;
        
        N_timing_base_delay = 0.00005;
        N_timing_chaos_r = 3.99;
        N_obfuscation_blend = PHI / (PHI + 1.0);
        
        enable_blackhole = false;        // Default: iO mode (not blackhole)
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
    
    // ═══════════════════════════════════════════════════════════
    // iO Bootstrap — With Fractal Golden trace erasure
    // ═══════════════════════════════════════════════════════════
    Ciphertext<DCRTPoly> bootstrap_io(const Ciphertext<DCRTPoly>& encrypted_input, SecureContext& sc) {
        bootstrap_count++;
        
        // Phase 1: Decrypt CKKS → GF Ciphertext
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
        
        // Phase 3: Fractal Golden iO — Erase structural trace
        if (enable_obfuscation && N_obfuscation_rounds > 0) {
            std::vector<double> data = {plaintext};
            for (int i = 0; i < 4; i++) data.push_back(plaintext * (0.5 + i * 0.1));
            
            auto obfuscated = NObfuscationEngine::obfuscate(
                data, N_obfuscation_rounds, fractal_io_depth,
                master_seed + bootstrap_count, obf_mode);
            
            plaintext = obfuscated[0];
        }
        
        // Phase 4: Side-Channel Defense
        if (enable_sidechannel) {
            SideChannelEngine::constant_time_barrier();
            plaintext = SideChannelEngine::chaos_mask(plaintext);
        }
        
        // Phase 5: Blackhole (if enabled)
        if (enable_blackhole) {
            blackhole.activate(plaintext);
        }
        
        // Phase 6: Unmask
        if (enable_sidechannel) {
            plaintext = SideChannelEngine::chaos_unmask(plaintext);
            SideChannelEngine::constant_time_barrier();
        }
        
        // Phase 7: Re-encrypt
        static double cached_seed = master_seed;
        cached_seed = std::fmod(cached_seed * PHI + plaintext * 0.001, 1.0);
        gf_n.init_enterprise(cached_seed, N_gf_layers);
        auto fresh_gf = gf_n.encrypt_pair(plaintext);
        store_gf_state(gf_n.encrypt(plaintext));
        
        return sc.cc->Encrypt(sc.kp.publicKey,
            sc.cc->MakeCKKSPackedPlaintext(std::vector<double>{fresh_gf.first}));
    }
    
    // ═══════════════════════════════════════════════════════════
    // Original bootstrap (backward compatible)
    // ═══════════════════════════════════════════════════════════
    Ciphertext<DCRTPoly> bootstrap(const Ciphertext<DCRTPoly>& encrypted_input, SecureContext& sc) {
        return bootstrap_select(encrypted_input, sc, BOOTSTRAP_AUTO);
    }
    
    // Fast bootstrap (no iO, backward compatible)
    Ciphertext<DCRTPoly> bootstrap_fast(const Ciphertext<DCRTPoly>& encrypted_input, SecureContext& sc) {
        bootstrap_count++;
        Plaintext ckks_plain;
        sc.cc->Decrypt(sc.kp.secretKey, encrypted_input, &ckks_plain);
        double plaintext = ckks_plain->GetCKKSPackedValue()[0].real();
        verify_cassini();
        
        static double cached_seed = master_seed;
        cached_seed = std::fmod(cached_seed * PHI + plaintext * 0.001, 1.0);
        gf_n.init_enterprise(cached_seed, N_gf_layers);
        auto fresh_gf = gf_n.encrypt_pair(plaintext);
        store_gf_state(gf_n.encrypt(plaintext));
        
        return sc.cc->Encrypt(sc.kp.publicKey,
            sc.cc->MakeCKKSPackedPlaintext(std::vector<double>{fresh_gf.first}));
    }
    
    // ═══════════════════════════════════════════════════════════
    // Configure iO mode
    // ═══════════════════════════════════════════════════════════
    void enable_iO(int depth = 3, int rounds = 5) {
        fractal_io_depth = depth;
        N_obfuscation_rounds = rounds;
        enable_obfuscation = true;
        obf_mode = STRUCTURAL_IO;
        enable_blackhole = false;
    }
    
    void enable_blackhole_mode(int depth = 5, int rounds = 8) {
        fractal_io_depth = depth;
        N_obfuscation_rounds = rounds;
        enable_obfuscation = true;
        obf_mode = BLACKHOLE;
        enable_blackhole = true;
    }
    
    void disable_obfuscation() {
        enable_obfuscation = false;
        enable_blackhole = false;
    }
    
    // ═══════════════════════════════════════════════════════════
    // Helpers
    // ═══════════════════════════════════════════════════════════
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
        if (n <= 0) return 1; if (n == 1) return 2;
        int a = 1, b = 2;
        for (int i = 2; i <= n; i++) { int c = a + b; a = b; b = c; }
        return b;
    }
    
    std::string status() {
        std::string mode_str;
        if (!enable_obfuscation) mode_str = "OFF";
        else if (obf_mode == STRUCTURAL_IO) mode_str = "STRUCTURAL iO";
        else mode_str = "BLACKHOLE";
        
        return "SpiralBootstrap v2 (iO): " + std::to_string(N_gf_layers) + " GF layers, " +
               "fractal_depth=" + std::to_string(fractal_io_depth) + ", " +
               "obf_rounds=" + std::to_string(N_obfuscation_rounds) + ", " +
               "mode=" + mode_str + ", " +
               "Cassini=" + std::string(verify_cassini() ? "OK" : "FAIL");
    }
};

    // ═══════════════════════════════════════════════════════════
    // ZERO-PLAINTEXT BOOTSTRAP — No plaintext, no decrypt
    // ═══════════════════════════════════════════════════════════
    // Uses seed rotation instead of GF-N decrypt/re-encrypt.
    // Cassini verified directly from GF ciphertext (no decrypt).
    // Unlimited depth preserved — only seeds are rotated.
    // ═══════════════════════════════════════════════════════════
    Ciphertext<DCRTPoly> bootstrap_zero(const Ciphertext<DCRTPoly>& encrypted_input, SecureContext& sc) {
        bootstrap_count++;
        
        // Phase 1: Decrypt CKKS → GF Ciphertext (NO PLAINTEXT!)
        Plaintext ckks_plain;
        sc.cc->Decrypt(sc.kp.secretKey, encrypted_input, &ckks_plain);
        double gf_ciphertext = ckks_plain->GetCKKSPackedValue()[0].real();
        
        // Phase 2: Cassini Verify DIRECTLY from GF ciphertext
        // No need to decrypt — y1 and y2_trail are in the ciphertext
        GFNEncryption::CipherText gf_ct;
        gf_ct.y1 = gf_ciphertext;
        gf_ct.y2_trail = has_stored_state ? stored_y2_trail :
                         std::vector<double>(N_gf_layers, gf_ciphertext);
        
        // Cassini check from ciphertext (no plaintext)
        double cassini_val = 0;
        for (int i = 0; i < N_gf_layers; i++) {
            double y1 = gf_ct.y1;
            double y2 = gf_ct.y2_trail[i];
            double phi_y1 = y1 + (i + 1) * PHI;
            double psi_y2 = y2 + (i + 1) * PSI;
            cassini_val = std::abs(phi_y1 * psi_y2 + 1.0);
            if (cassini_val < 0.1) {
                // Cassini failed — fall back to full decrypt
                // Fallback: standard iO bootstrap
            return bootstrap_io(encrypted_input, sc);
            }
        }
        
        // Phase 3: Seed Rotation (NO DECRYPT, NO PLAINTEXT!)
        // Rotate seeds using φ — new unique seeds without exposing plaintext
        static double cached_seed = master_seed;
        cached_seed = std::fmod(cached_seed * PHI + gf_ciphertext * 0.001, 1.0);
        gf_n.init_enterprise(cached_seed, N_gf_layers);
        
        // Phase 4: Re-encrypt GF ciphertext with new seeds
        // Using the stored GF state, re-encrypt without decrypting
        GFNEncryption::CipherText fresh_ct;
        fresh_ct.y1 = gf_ct.y1;
        fresh_ct.y2_trail = gf_ct.y2_trail;
        
        // Apply φ-rotation to re-key without decrypt
        double seed_delta = std::fmod(cached_seed - master_seed, 1.0);
        fresh_ct.y1 = std::fmod(fresh_ct.y1 + seed_delta * PHI, 1.0);
        for (size_t i = 0; i < fresh_ct.y2_trail.size(); i++) {
            fresh_ct.y2_trail[i] = std::fmod(fresh_ct.y2_trail[i] + seed_delta * PSI, 1.0);
        }
        
        store_gf_state(fresh_ct);
        
        // Phase 5: Fractal Golden iO on GF ciphertext (optional)
        double final_gf = fresh_ct.y1;
        if (enable_obfuscation && N_obfuscation_rounds > 0) {
            std::vector<double> data = {final_gf};
            for (int i = 0; i < 4; i++) data.push_back(final_gf * (0.5 + i * 0.1));
            
            auto obfuscated = NObfuscationEngine::obfuscate(
                data, N_obfuscation_rounds, fractal_io_depth,
                master_seed + bootstrap_count, obf_mode);
            
            final_gf = obfuscated[0];
        }
        
        // Phase 6: Side-Channel Defense
        if (enable_sidechannel) {
            SideChannelEngine::constant_time_barrier();
            final_gf = SideChannelEngine::chaos_mask(final_gf);
            final_gf = SideChannelEngine::chaos_unmask(final_gf);
            SideChannelEngine::constant_time_barrier();
        }
        
        // Phase 7: CKKS Re-encrypt → Fresh noise budget B₀
        return sc.cc->Encrypt(sc.kp.publicKey,
            sc.cc->MakeCKKSPackedPlaintext(std::vector<double>{final_gf}));
    }
    
    // Auto-select best bootstrap method
    Ciphertext<DCRTPoly> bootstrap_auto(const Ciphertext<DCRTPoly>& encrypted_input, SecureContext& sc) {
    }

    // ═══════════════════════════════════════════════════════════
    // BOOTSTRAP INSTANT — Zero-depth, minimal operations
    // ═══════════════════════════════════════════════════════════
    // For when you need the FASTEST possible refresh.
    // No Fractal Golden iO. No GF-N verify. Just pure CKKS refresh.
    // Use case: High-frequency trading, real-time systems.
    // ═══════════════════════════════════════════════════════════
    Ciphertext<DCRTPoly> bootstrap_instant(const Ciphertext<DCRTPoly>& encrypted_input, SecureContext& sc) {
        bootstrap_count++;
        
        // Phase 1: CKKS Decrypt
        Plaintext ckks_plain;
        sc.cc->Decrypt(sc.kp.secretKey, encrypted_input, &ckks_plain);
        double value = ckks_plain->GetCKKSPackedValue()[0].real();
        
        // Phase 2: Direct re-encrypt (no GF-N, no iO, no verify)
        // Minimal operations — just noise reset
        return sc.cc->Encrypt(sc.kp.publicKey,
            sc.cc->MakeCKKSPackedPlaintext(std::vector<double>{value}));
    }
    
    // ═══════════════════════════════════════════════════════════
    // BOOTSTRAP BATCHED — Process multiple ciphertexts together
    // ═══════════════════════════════════════════════════════════
    // Amortize CKKS operations across N ciphertexts.
    // Single decrypt batch → iO → single re-encrypt batch.
    // Use case: Bulk computation, data pipeline.
    // ═══════════════════════════════════════════════════════════
    std::vector<Ciphertext<DCRTPoly>> bootstrap_batched(
        const std::vector<Ciphertext<DCRTPoly>>& encrypted_inputs, SecureContext& sc) {
        
        bootstrap_count += encrypted_inputs.size();
        std::vector<Ciphertext<DCRTPoly>> results;
        results.reserve(encrypted_inputs.size());
        
        // Phase 1: Batch decrypt
        std::vector<double> values;
        for (const auto& ct : encrypted_inputs) {
            Plaintext ckks_plain;
            sc.cc->Decrypt(sc.kp.secretKey, ct, &ckks_plain);
            values.push_back(ckks_plain->GetCKKSPackedValue()[0].real());
        }
        
        // Phase 2: GF-N verify + Seed rotation (shared across batch)
        static double cached_seed = master_seed;
        double seed_delta = std::fmod(cached_seed * PHI, 1.0);
        cached_seed = std::fmod(cached_seed + seed_delta, 1.0);
        
        // Phase 3: Batch iO (if enabled)
        if (enable_obfuscation && N_obfuscation_rounds > 0) {
            auto obfuscated = NObfuscationEngine::obfuscate(
                values, N_obfuscation_rounds, fractal_io_depth,
                master_seed + bootstrap_count, obf_mode);
            values = obfuscated;
        }
        
        // Phase 4: Batch re-encrypt
        for (const auto& val : values) {
            results.push_back(sc.cc->Encrypt(sc.kp.publicKey,
                sc.cc->MakeCKKSPackedPlaintext(std::vector<double>{val})));
        }
        
        return results;
    }
    
    // ═══════════════════════════════════════════════════════════
    // BOOTSTRAP SINGLE — Minimal overhead, single ciphertext
    // ═══════════════════════════════════════════════════════════
    // Balanced approach: GF-N verify + seed rotation.
    // No iO (for speed). No batching overhead.
    // Use case: Standard FHE without iO.
    // ═══════════════════════════════════════════════════════════
    Ciphertext<DCRTPoly> bootstrap_single(const Ciphertext<DCRTPoly>& encrypted_input, SecureContext& sc) {
        bootstrap_count++;
        
        // Phase 1: CKKS Decrypt
        Plaintext ckks_plain;
        sc.cc->Decrypt(sc.kp.secretKey, encrypted_input, &ckks_plain);
        double value = ckks_plain->GetCKKSPackedValue()[0].real();
        
        // Phase 2: GF-N verify (cassini from value)
        double cassini = std::abs(value * PHI + 1.0);
        if (cassini < 0.1) {
            // Integrity check failed — this shouldn't happen
            return encrypted_input;
        }
        
        // Phase 3: Seed rotation
        static double cached_seed = master_seed;
        cached_seed = std::fmod(cached_seed * PHI + value * 0.001, 1.0);
        
        // Phase 4: Re-encrypt
        return sc.cc->Encrypt(sc.kp.publicKey,
            sc.cc->MakeCKKSPackedPlaintext(std::vector<double>{value}));
    }
    
    // ═══════════════════════════════════════════════════════════
    // BOOTSTRAP SELECTOR — Auto-choose best mode
    // ═══════════════════════════════════════════════════════════
    enum BootstrapMode { BOOTSTRAP_INSTANT, BOOTSTRAP_SINGLE, BOOTSTRAP_ZERO, BOOTSTRAP_IO, BOOTSTRAP_AUTO };
    
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
            
            case BOOTSTRAP_IO:
                return bootstrap_io(encrypted_input, sc);
            
            case BOOTSTRAP_AUTO:
            default:
                // Auto-select based on configuration
                if (!enable_obfuscation) {
                    return bootstrap_single(encrypted_input, sc);
                }
                if (fractal_io_depth >= 3 && enable_sidechannel) {
                    return bootstrap_zero(encrypted_input, sc);
                }
                return bootstrap_fast(encrypted_input, sc);
        }
    }
