#pragma once
#include <cmath>
#include <cstdint>

namespace SpiralFHE {

constexpr double PHI = 1.6180339887498948482;
constexpr double PSI = -0.6180339887498948482;
constexpr double TAU = 0.1;
constexpr double SEED_WEIGHT = 0.001;
constexpr int SUPPORTED_A[] = {1, 3, 4, 5, 6, 7};
constexpr int MAX_LAYERS = 6;

struct GFNLayerCore {
    double y1, y2, seed, cassini;
    int a_value;
    bool valid;
};

template<int N>
class GFNCore {
private:
    GFNLayerCore layers[N];
    double master_seed;
    double cached_seed;
    int rotation_count;
    static_assert(N <= MAX_LAYERS, "Max 6 layers");

public:
    GFNCore() : master_seed(0.0), cached_seed(0.0), rotation_count(0) {}

    void initialize(double seed) {
        master_seed = seed;
        cached_seed = seed;
        double cs = seed;
        for (int i = 0; i < N; i++) {
            int a = SUPPORTED_A[i];
            layers[i].seed = cs;
            layers[i].a_value = a;
            layers[i].y1 = sin(cs * PHI);
            layers[i].y2 = cos(cs * PSI);
            layers[i].cassini = fabs((layers[i].y1 + a * PHI) * 
                                     (layers[i].y2 + a * PSI) + 1.0);
            layers[i].valid = (layers[i].cassini > TAU);
            cs = fmod(cs * PHI + 0.618, 1.0);
        }
        rotation_count = 0;
    }

    double get_min_cassini() const {
        double m = 1e10;
        for (int i = 0; i < N; i++) {
            if (layers[i].cassini < m) m = layers[i].cassini;
        }
        return m;
    }

    bool is_healthy() const {
        for (int i = 0; i < N; i++)
            if (layers[i].cassini <= TAU) return false;
        return true;
    }

    int healthy_count() const {
        int c = 0;
        for (int i = 0; i < N; i++)
            if (layers[i].cassini > TAU) c++;
        return c;
    }

    void rotate_seeds(double value) {
        double new_seed = fmod(cached_seed * PHI + fabs(value) * SEED_WEIGHT, 1.0);
        cached_seed = new_seed;
        double cs = cached_seed;
        for (int i = 0; i < N; i++) {
            int a = SUPPORTED_A[i];
            layers[i].seed = cs;
            layers[i].a_value = a;
            layers[i].y1 = sin(cs * PHI);
            layers[i].y2 = cos(cs * PSI);
            layers[i].cassini = fabs((layers[i].y1 + a * PHI) * 
                                     (layers[i].y2 + a * PSI) + 1.0);
            layers[i].valid = (layers[i].cassini > TAU);
            cs = fmod(cs * PHI + 0.618, 1.0);
        }
        rotation_count++;
    }

    double get_cached_seed() const { return cached_seed; }
    int get_rotation_count() const { return rotation_count; }
    constexpr int get_layer_count() const { return N; }
};

}
