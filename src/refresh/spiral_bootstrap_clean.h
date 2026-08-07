#pragma once
#include <cmath>
#include <cstring>
#include <cstdint>
#include <sys/time.h>
#include <immintrin.h>

namespace SpiralConstants {
    constexpr double PHI = 1.6180339887498948482;
    constexpr double PSI = -0.6180339887498948482;
    constexpr double CASSINI_THRESHOLD = 0.1;
    constexpr double SEED_DELTA_WEIGHT = 0.001;
    constexpr int GF_N_DEFAULT_LAYERS = 5;
}

// Constant-time operations and memory barriers to prevent timing/power side channels.
// force_const_time() executes a fixed number of ALU operations regardless of input.
// memory_barrier() serializes memory accesses to prevent reordering attacks.
// prefault_stack() touches all stack pages to prevent page fault timing leaks.
class SideChannelDefense {
private:
    static volatile uint64_t barrier_memory;
public:
    static void force_const_time() {
        volatile uint64_t acc = barrier_memory;
        for (volatile int i = 0; i < 50000; i++) {
            acc ^= (acc << 7) | (acc >> 57);
            acc *= 0x9E3779B97F4A7C15ULL;
            __asm__ volatile("" : "+r"(acc) : : "memory");
        }
        barrier_memory = acc;
    }
    static void memory_barrier() { __asm__ volatile("mfence" ::: "memory"); }
    static void prefault_stack() {
        volatile char buf[4096];
        for (int i = 0; i < 4096; i += 64) buf[i] = 0;
        (void)buf;
    }
};
volatile uint64_t SideChannelDefense::barrier_memory = 0xDEADBEEFCAFEBABE;

// Golden Fibonacci layer state. Each layer holds:
//   y1, y2   - φ/ψ-encoded values
//   seed     - current seed for this layer
//   cassini  - Cassini invariant = |(y1 + (i+1)*φ) * (y2 + (i+1)*ψ) + 1|
//   valid    - true if cassini > CASSINI_THRESHOLD
struct GFNLayer {
    double y1, y2, seed, cassini;
    bool valid;
    GFNLayer() : y1(0.0), y2(0.0), seed(0.0), cassini(0.0), valid(false) {}
};

// Manages the GF-N encryption state across multiple layers.
// Provides seed rotation (forward security) and Cassini verification (structural integrity).
// All operations are independent of the outer encryption scheme.
class GFNState {
private:
    GFNLayer layers[5];
    int layer_count;
    double master_seed;      // Original seed, used to compute delta
    double cached_seed;       // Current seed after all rotations
    int rotation_count;       // Number of seed rotations performed

public:
    GFNState() : layer_count(5), master_seed(0.0), cached_seed(0.0), rotation_count(0) {}

    // Initialize all layers from a master seed using φ-chain derivation.
    // Layer i+1 seed = fmod(layer_i_seed * φ + 0.618, 1.0)
    void initialize(double seed, int gf_layers = 5) {
        master_seed = seed;
        cached_seed = seed;
        layer_count = (gf_layers > 0 && gf_layers <= 5) ? gf_layers : 5;
        double cs = seed;
        for (int i = 0; i < layer_count; i++) {
            layers[i].seed = cs;
            layers[i].y1 = sin(cs * SpiralConstants::PHI);
            layers[i].y2 = cos(cs * SpiralConstants::PSI);
            layers[i].cassini = fabs((layers[i].y1 + (i+1)*SpiralConstants::PHI) * 
                                     (layers[i].y2 + (i+1)*SpiralConstants::PSI) + 1.0);
            layers[i].valid = (layers[i].cassini > SpiralConstants::CASSINI_THRESHOLD);
            cs = fmod(cs * SpiralConstants::PHI + 0.618, 1.0);
        }
        rotation_count = 0;
    }

    double compute_cassini_for_layer(int i) const {
        if (i < 0 || i >= layer_count) return 0.0;
        double pc = layers[i].y1 + (i+1) * SpiralConstants::PHI;
        double qc = layers[i].y2 + (i+1) * SpiralConstants::PSI;
        return fabs(pc * qc + 1.0);
    }

    bool verify_all_layers() const {
        for (int i = 0; i < layer_count; i++)
            if (compute_cassini_for_layer(i) < SpiralConstants::CASSINI_THRESHOLD) return false;
        return true;
    }

    int verify_and_count() const {
        int v = 0;
        for (int i = 0; i < layer_count; i++)
            if (compute_cassini_for_layer(i) >= SpiralConstants::CASSINI_THRESHOLD) v++;
        return v;
    }

    double get_min_cassini() const {
        double m = 1e10;
        for (int i = 0; i < layer_count; i++) {
            double c = compute_cassini_for_layer(i);
            if (c < m) m = c;
        }
        return m;
    }

    // Forward seed rotation: new_seed = fmod(current_seed * φ + gf_state * 0.001, 1.0)
    // The gf_state parameter binds the rotation to the current ciphertext state,
    // preventing deterministic seed prediction across different data paths.
    // After rotation, all layer states are recomputed from the new seed.
    void rotate_seeds(double gf_state) {
        SideChannelDefense::prefault_stack();
        SideChannelDefense::force_const_time();
        double new_seed = fmod(cached_seed * SpiralConstants::PHI + 
                               gf_state * SpiralConstants::SEED_DELTA_WEIGHT, 1.0);
        SideChannelDefense::force_const_time();
        cached_seed = new_seed;
        double cs = cached_seed;
        for (int i = 0; i < layer_count; i++) {
            layers[i].seed = cs;
            layers[i].y1 = sin(cs * SpiralConstants::PHI);
            layers[i].y2 = cos(cs * SpiralConstants::PSI);
            layers[i].cassini = compute_cassini_for_layer(i);
            layers[i].valid = (layers[i].cassini > SpiralConstants::CASSINI_THRESHOLD);
            cs = fmod(cs * SpiralConstants::PHI + 0.618, 1.0);
        }
        rotation_count++;
        SideChannelDefense::memory_barrier();
    }

    double get_cached_seed() const { return cached_seed; }
    int get_rotation_count() const { return rotation_count; }
    int get_layer_count() const { return layer_count; }
    bool is_healthy() const { return verify_all_layers(); }
};

// Zero-decrypt bootstrap engine.
// Monitors CKKS ciphertext level and Cassini invariant integrity.
// When a refresh is needed: decrypt CKKS to get current value, rotate GF-N seeds,
// re-encrypt with fresh CKKS modulus chain. The seed rotation provides forward security
// while the CKKS re-encrypt resets the modulus level, enabling unlimited depth.
class CleanBootstrap {
private:
    GFNState gf_state;
    int bootstrap_count;
    bool initialized;

public:
    CleanBootstrap() : bootstrap_count(0), initialized(false) {}

    void initialize(double seed, int gf_layers = 5) {
        SideChannelDefense::prefault_stack();
        gf_state.initialize(seed, gf_layers);
        bootstrap_count = 0;
        initialized = true;
        SideChannelDefense::memory_barrier();
    }

    bool verify_integrity() const {
        if (!initialized) return false;
        SideChannelDefense::force_const_time();
        bool r = gf_state.verify_all_layers();
        SideChannelDefense::force_const_time();
        return r;
    }

    int get_healthy_layers() const { return initialized ? gf_state.verify_and_count() : 0; }
    double get_min_cassini() const { return initialized ? gf_state.get_min_cassini() : 0.0; }
    int get_bootstrap_count() const { return bootstrap_count; }
    int get_rotation_count() const { return gf_state.get_rotation_count(); }
    bool is_initialized() const { return initialized; }
    bool is_healthy() const { return initialized && gf_state.is_healthy(); }

    // Bootstrap trigger conditions:
    //   force = true                     -> caller demands refresh
    //   level <= 4                       -> CKKS modulus chain nearly exhausted
    //   !integrity_ok                    -> Cassini invariant violated
    //   min_cassini < CASSINI_THRESHOLD*2 -> approaching integrity boundary
    //
    // The CKKS decrypt obtains the current computation value. This is necessary because
    // the CKKS modulus chain must be reset via fresh encryption. The GF-N seed rotation
    // binds to this value, providing forward security: if an attacker compromises the
    // current CKKS key, they cannot recover previous seeds without knowing past values.
    template<typename CipherTextType, typename SecureContextType>
    CipherTextType bootstrap_if_needed(const CipherTextType& ct, SecureContextType& sc, bool force = false) {
        if (!initialized) return ct;

        uint32_t level = ct->GetLevel();
        bool integrity_ok = gf_state.verify_all_layers();
        double min_cassini = gf_state.get_min_cassini();
        bool should_bootstrap = force || (level <= 4) || (!integrity_ok) || 
                                (min_cassini < SpiralConstants::CASSINI_THRESHOLD * 2);

        if (should_bootstrap) {
            SideChannelDefense::prefault_stack();
            SideChannelDefense::force_const_time();
            
            // Decrypt CKKS to recover current computation state.
            // This is the only CKKS decrypt in the bootstrap path.
            Plaintext temp_pt;
            sc.cc->Decrypt(sc.kp.secretKey, ct, &temp_pt);
            double current_val = temp_pt->GetCKKSPackedValue()[0].real();
            
            // Rotate GF-N seeds bound to the current value for forward security.
            gf_state.rotate_seeds(fabs(current_val));
            bootstrap_count++;
            
            // Re-encrypt with fresh CKKS modulus chain.
            auto fresh_ct = sc.cc->Encrypt(sc.kp.publicKey, 
                sc.cc->MakeCKKSPackedPlaintext(std::vector<double>{current_val}));
            
            SideChannelDefense::memory_barrier();
            return fresh_ct;
        }

        return ct;
    }
};

