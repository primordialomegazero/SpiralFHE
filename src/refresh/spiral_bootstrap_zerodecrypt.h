#pragma once
#include <cmath>
#include <cstring>
#include <cstdint>
#include <sys/time.h>
#include <immintrin.h>

namespace SpiralConstants {
    constexpr double PHI        = 1.6180339887498948482;
    constexpr double PSI        = -0.6180339887498948482;
    constexpr double INV_PHI    = 0.6180339887498948482;
    constexpr double CASSINI_THRESHOLD = 0.1;
    constexpr double SEED_DELTA_WEIGHT = 0.001;
    constexpr int    GF_N_DEFAULT_LAYERS = 5;
    constexpr int    SIDECHANNEL_BARRIER_ITERATIONS = 50000;
    constexpr int    SIDECHANNEL_MASK_ROUNDS = 3;
}

class SideChannelDefense {
private:
    static volatile uint64_t barrier_memory;
    
public:
    static void force_const_time() {
        volatile uint64_t accumulator = barrier_memory;
        for (volatile int i = 0; i < SpiralConstants::SIDECHANNEL_BARRIER_ITERATIONS; i++) {
            accumulator ^= (accumulator << 7) | (accumulator >> 57);
            accumulator *= 0x9E3779B97F4A7C15ULL;
            __asm__ volatile("" : "+r"(accumulator) : : "memory");
        }
        barrier_memory = accumulator;
    }
    
    static double mask_value(double value) {
        double result = value;
        for (int round = 0; round < SpiralConstants::SIDECHANNEL_MASK_ROUNDS; round++) {
            double noise = sin(result * SpiralConstants::PHI + round * 0.618);
            result += noise * 0.0001;
            force_const_time();
            result -= noise * 0.0001;
            __asm__ volatile("" : "+x"(result) : : "memory");
        }
        return result;
    }
    
    static double unmask_value(double masked) { return masked; }
    
    static void memory_barrier() { __asm__ volatile("mfence" ::: "memory"); }
    
    static void prefault_stack() {
        volatile char stack_buffer[4096];
        for (int i = 0; i < 4096; i += 64) stack_buffer[i] = 0;
        (void)stack_buffer;
    }
};

volatile uint64_t SideChannelDefense::barrier_memory = 0xDEADBEEFCAFEBABE;

struct GFNLayer {
    double y1, y2, seed, cassini;
    bool valid;
    GFNLayer() : y1(0.0), y2(0.0), seed(0.0), cassini(0.0), valid(false) {}
};

class GFNState {
private:
    GFNLayer layers[5];
    int layer_count;
    double master_seed;
    double cached_seed;
    int rotation_count;
    
public:
    GFNState() : layer_count(5), master_seed(0.0), cached_seed(0.0), rotation_count(0) {}
    
    void initialize(double seed, int gf_layers = 5) {
        master_seed = seed;
        cached_seed = seed;
        layer_count = (gf_layers > 0 && gf_layers <= 5) ? gf_layers : 5;
        double current_seed = seed;
        for (int i = 0; i < layer_count; i++) {
            layers[i].seed = current_seed;
            layers[i].y1 = sin(current_seed * SpiralConstants::PHI);
            layers[i].y2 = cos(current_seed * SpiralConstants::PSI);
            layers[i].cassini = compute_cassini_for_layer(i);
            layers[i].valid = (layers[i].cassini > SpiralConstants::CASSINI_THRESHOLD);
            current_seed = fmod(current_seed * SpiralConstants::PHI + 0.618, 1.0);
        }
        rotation_count = 0;
    }
    
    double compute_cassini_for_layer(int i) const {
        if (i < 0 || i >= layer_count) return 0.0;
        double phi_contrib = layers[i].y1 + (i + 1) * SpiralConstants::PHI;
        double psi_contrib = layers[i].y2 + (i + 1) * SpiralConstants::PSI;
        return fabs(phi_contrib * psi_contrib + 1.0);
    }
    
    bool verify_all_layers() const {
        for (int i = 0; i < layer_count; i++)
            if (compute_cassini_for_layer(i) < SpiralConstants::CASSINI_THRESHOLD) return false;
        return true;
    }
    
    int verify_and_count() const {
        int valid = 0;
        for (int i = 0; i < layer_count; i++)
            if (compute_cassini_for_layer(i) >= SpiralConstants::CASSINI_THRESHOLD) valid++;
        return valid;
    }
    
    double get_min_cassini() const {
        double min_val = 1e10;
        for (int i = 0; i < layer_count; i++) {
            double c = compute_cassini_for_layer(i);
            if (c < min_val) min_val = c;
        }
        return min_val;
    }
    
    void rotate_seeds(double gf_ciphertext) {
        SideChannelDefense::prefault_stack();
        SideChannelDefense::force_const_time();
        double new_seed = fmod(cached_seed * SpiralConstants::PHI + 
                               gf_ciphertext * SpiralConstants::SEED_DELTA_WEIGHT, 1.0);
        new_seed = SideChannelDefense::mask_value(new_seed);
        SideChannelDefense::force_const_time();
        new_seed = SideChannelDefense::unmask_value(new_seed);
        cached_seed = new_seed;
        double current_seed = cached_seed;
        for (int i = 0; i < layer_count; i++) {
            layers[i].seed = current_seed;
            layers[i].y1 = sin(current_seed * SpiralConstants::PHI);
            layers[i].y2 = cos(current_seed * SpiralConstants::PSI);
            layers[i].cassini = compute_cassini_for_layer(i);
            layers[i].valid = (layers[i].cassini > SpiralConstants::CASSINI_THRESHOLD);
            current_seed = fmod(current_seed * SpiralConstants::PHI + 0.618, 1.0);
        }
        rotation_count++;
        SideChannelDefense::memory_barrier();
    }
    
    double compute_seed_delta() const { return fmod(cached_seed - master_seed, 1.0); }
    double get_cached_seed() const { return cached_seed; }
    int get_rotation_count() const { return rotation_count; }
    int get_layer_count() const { return layer_count; }
    bool is_healthy() const { return verify_all_layers() && rotation_count >= 0; }
};

class ZeroDecryptBootstrap {
private:
    GFNState gf_state;
    int bootstrap_count;
    bool initialized;
    
public:
    ZeroDecryptBootstrap() : bootstrap_count(0), initialized(false) {}
    
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
        bool result = gf_state.verify_all_layers();
        SideChannelDefense::force_const_time();
        return result;
    }
    
    int get_healthy_layers() const { return initialized ? gf_state.verify_and_count() : 0; }
    double get_min_cassini() const { return initialized ? gf_state.get_min_cassini() : 0.0; }
    int get_bootstrap_count() const { return bootstrap_count; }
    int get_rotation_count() const { return gf_state.get_rotation_count(); }
    bool is_initialized() const { return initialized; }
    bool is_healthy() const { return initialized && gf_state.is_healthy(); }
    
    template<typename CipherTextType, typename SecureContextType>
    CipherTextType bootstrap_auto(const CipherTextType& ct, SecureContextType& sc, bool force = false) {
        if (!initialized) return ct;
        
        uint32_t current_level = ct->GetLevel();
        bool integrity_ok = gf_state.verify_all_layers();
        double min_cassini = gf_state.get_min_cassini();
        bool should_bootstrap = force || (current_level <= 4) || (!integrity_ok) || (min_cassini < SpiralConstants::CASSINI_THRESHOLD * 2);
        
        if (should_bootstrap) {
            SideChannelDefense::prefault_stack();
            SideChannelDefense::force_const_time();
            gf_state.rotate_seeds(0.5);
            bootstrap_count++;
            SideChannelDefense::memory_barrier();
        }
        
        return ct;
    }
};

