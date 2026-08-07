#pragma once
#include <cmath>
#include <cstdint>
#include <deque>
#include <vector>
#include <algorithm>
#include <numeric>
#include <sys/time.h>
#include <immintrin.h>
#include "../core/constants.h"

// Side channel defense: constant-time operations and memory barriers.
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

// GF-N Layer: Golden Fibonacci encryption layer with Cassini invariant.
struct GFNLayer {
    double y1, y2, seed, cassini;
    bool valid;
    GFNLayer() : y1(0.0), y2(0.0), seed(0.0), cassini(0.0), valid(false) {}
};

template<int N>
class GFNState {
private:
    GFNLayer layers[N];
    double master_seed;
    double cached_seed;
    int rotation_count;

public:
    GFNState() : master_seed(0.0), cached_seed(0.0), rotation_count(0) {}

    void initialize(double seed) {
        master_seed = seed;
        cached_seed = seed;
        double cs = seed;
        for (int i = 0; i < N; i++) {
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
        if (i < 0 || i >= N) return 0.0;
        double pc = layers[i].y1 + (i+1) * SpiralConstants::PHI;
        double qc = layers[i].y2 + (i+1) * SpiralConstants::PSI;
        return fabs(pc * qc + 1.0);
    }

    bool verify_all_layers() const {
        for (int i = 0; i < N; i++)
            if (compute_cassini_for_layer(i) < SpiralConstants::CASSINI_THRESHOLD) return false;
        return true;
    }

    int verify_and_count() const {
        int v = 0;
        for (int i = 0; i < N; i++)
            if (compute_cassini_for_layer(i) >= SpiralConstants::CASSINI_THRESHOLD) v++;
        return v;
    }

    double get_min_cassini() const {
        double m = 1e10;
        for (int i = 0; i < N; i++) {
            double c = compute_cassini_for_layer(i);
            if (c < m) m = c;
        }
        return m;
    }

    void rotate_seeds(double gf_state) {
        SideChannelDefense::prefault_stack();
        SideChannelDefense::force_const_time();
        double new_seed = fmod(cached_seed * SpiralConstants::PHI + 
                               gf_state * SpiralConstants::SEED_DELTA_WEIGHT, 1.0);
        SideChannelDefense::force_const_time();
        cached_seed = new_seed;
        double cs = cached_seed;
        for (int i = 0; i < N; i++) {
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
    constexpr int get_layer_count() const { return N; }
    bool is_healthy() const { return verify_all_layers(); }
};

// Operational state snapshot for the controller.
struct OperationalState {
    double cassini_min;
    int healthy_layers;
    int total_layers;
    uint32_t ckks_level;
    uint32_t ckks_max_level;
    int ops_since_bootstrap;
};

// Controller state: PHI metric and bootstrap decision.
struct ControllerState {
    double integrated_phi;
    double stability;
    int healthy_layers;
    bool should_bootstrap;
};

// Meta-controller state: parameter tuning.
struct MetaControllerState {
    double phi_convergence_rate;
    double threshold_stability;
    double learning_rate;
    double adjusted_threshold;
    uint32_t learned_min_level;
    bool parameters_stable;
};

// Fractal detector: self-similarity detection for convergence.
template<size_t WindowSize = 20>
class FractalDetector {
private:
    static constexpr double SIMILARITY_THRESHOLD = 0.95;
    std::deque<double> similarity_history;
    bool fractal_detected;
    int depth_reached;

public:
    FractalDetector() : fractal_detected(false), depth_reached(0) {}

    double compute_correlation(const std::deque<double>& a, const std::deque<double>& b) {
        if (a.size() < 5 || b.size() < 5) return 0.0;
        size_t n = std::min(a.size(), b.size());
        double mean_a = 0.0, mean_b = 0.0;
        for (size_t i = a.size() - n; i < a.size(); i++) mean_a += a[i];
        for (size_t i = b.size() - n; i < b.size(); i++) mean_b += b[i];
        mean_a /= n; mean_b /= n;
        double cov = 0.0, var_a = 0.0, var_b = 0.0;
        for (size_t i = 0; i < n; i++) {
            double da = a[a.size() - n + i] - mean_a;
            double db = b[b.size() - n + i] - mean_b;
            cov += da * db;
            var_a += da * da;
            var_b += db * db;
        }
        if (var_a < 1e-12 || var_b < 1e-12) return 0.0;
        return fabs(cov / sqrt(var_a * var_b));
    }

    bool detect(size_t depth, const std::deque<double>& level_n,
                const std::deque<double>& level_n_minus_1) {
        double sim = compute_correlation(level_n, level_n_minus_1);
        similarity_history.push_back(sim);
        if (similarity_history.size() > WindowSize) similarity_history.pop_front();
        if (sim > SIMILARITY_THRESHOLD) {
            fractal_detected = true;
            depth_reached = depth;
            return true;
        }
        return false;
    }

    bool is_fractal() const { return fractal_detected; }
    int get_depth() const { return depth_reached; }
    double last_similarity() const {
        return similarity_history.empty() ? 0.0 : similarity_history.back();
    }
};

// Recursive Fractal Controller: 3-level self-optimizing bootstrap controller.
// Level 1: Computes PHI from Cassini health and CKKS level.
// Level 2: Adjusts threshold and learning rate.
// Level 3: Detects fractal self-similarity.
template<size_t HistorySize = 30>
class RecursiveFractalController {
private:
    static constexpr double PHI = SpiralConstants::PHI;
    static constexpr double INV_PHI = 0.6180339887498948482;

    std::deque<ControllerState> level1_history;
    std::deque<MetaControllerState> level2_history;
    std::deque<double> level1_phi_series;
    std::deque<double> level2_convergence_series;
    std::deque<uint32_t> level_samples;
    MetaControllerState current_meta;
    FractalDetector<HistorySize> detector;

    double compute_phi_from_operational(const OperationalState& op) {
        double health_fraction = (op.total_layers > 0) ?
            static_cast<double>(op.healthy_layers) / op.total_layers : 0.0;

        double connectivity = 0.0;
        if (level1_history.size() >= 2) {
            for (size_t i = 1; i < level1_history.size(); i++) {
                connectivity += fabs(level1_history[i].integrated_phi -
                                     level1_history[i-1].integrated_phi);
            }
            connectivity /= (level1_history.size() - 1);
        }

        double cassini_signal = op.cassini_min;
        double cassini_penalty = (cassini_signal < SpiralConstants::CASSINI_THRESHOLD * 3) ?
            (1.0 - cassini_signal) : 0.0;

        double level_fraction = (op.ckks_max_level > 0) ?
            static_cast<double>(op.ckks_level) / op.ckks_max_level : 0.0;
        double level_penalty = (level_fraction < 0.5) ? (1.0 - level_fraction) : 0.0;

        double variance = 0.0;
        if (level1_history.size() >= 3) {
            double mean = 0.0;
            for (auto& s : level1_history) mean += s.integrated_phi;
            mean /= level1_history.size();
            for (auto& s : level1_history) {
                double d = s.integrated_phi - mean;
                variance += d * d;
            }
            variance /= level1_history.size();
        }

        return (connectivity * health_fraction * (1.0 + cassini_penalty + level_penalty)) / (1.0 + variance);
    }

    MetaControllerState compute_meta_state(const OperationalState& op) {
        MetaControllerState meta;
        if (level1_history.size() < 10) {
            meta.phi_convergence_rate = 0.0;
            meta.threshold_stability = 0.0;
            meta.learning_rate = INV_PHI;
            meta.adjusted_threshold = SpiralConstants::CASSINI_THRESHOLD * 3;
            meta.learned_min_level = 4;
            meta.parameters_stable = false;
            return meta;
        }

        size_t n = level1_history.size();
        auto r_end = level1_history.end();
        auto r_mid = level1_history.end() - std::min(size_t(5), n);
        auto p_beg = level1_history.end() - std::min(size_t(10), n);

        double recent_var = compute_variance(r_mid, r_end);
        double prior_var = compute_variance(p_beg, r_mid);
        meta.phi_convergence_rate = (prior_var > 1e-12)
            ? 1.0 - (recent_var / prior_var) : 0.0;

        meta.learning_rate = INV_PHI * (1.0 - meta.phi_convergence_rate) + 0.1;
        meta.threshold_stability = level1_history.back().stability;
        meta.adjusted_threshold = SpiralConstants::CASSINI_THRESHOLD * 
            (1.0 + meta.learning_rate);

        level_samples.push_back(op.ckks_level);
        if (level_samples.size() > HistorySize) level_samples.pop_front();

        if (level_samples.size() >= 5) {
            uint32_t min_seen = *std::min_element(level_samples.begin(), level_samples.end());
            uint32_t max_seen = *std::max_element(level_samples.begin(), level_samples.end());
            uint32_t range = (max_seen > min_seen) ? (max_seen - min_seen) : 1;
            meta.learned_min_level = min_seen + static_cast<uint32_t>(range * meta.learning_rate * 0.5);
            if (meta.learned_min_level < 2) meta.learned_min_level = 2;
            if (meta.learned_min_level > 8) meta.learned_min_level = 8;
        } else {
            meta.learned_min_level = 4;
        }

        meta.parameters_stable = (meta.threshold_stability > 0.9) &&
                                  (meta.phi_convergence_rate > 0.5);
        return meta;
    }

    double compute_variance(typename std::deque<ControllerState>::const_iterator begin,
                            typename std::deque<ControllerState>::const_iterator end) {
        if (begin == end) return 0.0;
        double mean = 0.0;
        size_t count = 0;
        for (auto it = begin; it != end; ++it, ++count) mean += it->integrated_phi;
        if (count == 0) return 0.0;
        mean /= count;
        double var = 0.0;
        for (auto it = begin; it != end; ++it) {
            double d = it->integrated_phi - mean;
            var += d * d;
        }
        return var / count;
    }

    bool detect_fractal() {
        if (level1_phi_series.size() < 10 || level2_convergence_series.size() < 10)
            return false;
        return detector.detect(3, level1_phi_series, level2_convergence_series);
    }

public:
    RecursiveFractalController() {
        current_meta.phi_convergence_rate = 0.0;
        current_meta.threshold_stability = 0.0;
        current_meta.learning_rate = INV_PHI;
        current_meta.adjusted_threshold = SpiralConstants::CASSINI_THRESHOLD * 3;
        current_meta.learned_min_level = 4;
        current_meta.parameters_stable = false;
    }

    ControllerState update(const OperationalState& op_state) {
        ControllerState ctrl;
        ctrl.integrated_phi = compute_phi_from_operational(op_state);
        ctrl.healthy_layers = op_state.healthy_layers;

        if (level1_history.size() >= 5) {
            std::vector<double> recent;
            size_t start = level1_history.size() - std::min(size_t(5), level1_history.size());
            for (size_t i = start; i < level1_history.size(); i++)
                recent.push_back(level1_history[i].integrated_phi);
            double mean = std::accumulate(recent.begin(), recent.end(), 0.0) / recent.size();
            ctrl.stability = 0.0;
            for (double v : recent)
                ctrl.stability += (v - mean) * (v - mean);
            ctrl.stability = 1.0 / (1.0 + ctrl.stability / recent.size());
        } else {
            ctrl.stability = 0.0;
        }

        bool ckks_critical = (op_state.ckks_level <= current_meta.learned_min_level);
        bool cassini_critical = (op_state.cassini_min < current_meta.adjusted_threshold);
        bool layers_degraded = (op_state.healthy_layers < op_state.total_layers);
        bool emergency = (op_state.cassini_min < SpiralConstants::CASSINI_THRESHOLD * 2);

        ctrl.should_bootstrap = ckks_critical || cassini_critical || layers_degraded || emergency;

        level1_history.push_back(ctrl);
        level1_phi_series.push_back(ctrl.integrated_phi);
        if (level1_history.size() > HistorySize) level1_history.pop_front();
        if (level1_phi_series.size() > HistorySize) level1_phi_series.pop_front();

        current_meta = compute_meta_state(op_state);
        level2_history.push_back(current_meta);
        level2_convergence_series.push_back(current_meta.phi_convergence_rate);
        if (level2_history.size() > HistorySize) level2_history.pop_front();
        if (level2_convergence_series.size() > HistorySize)
            level2_convergence_series.pop_front();

        detect_fractal();
        return ctrl;
    }

    ControllerState get_state() const {
        return level1_history.empty() ? ControllerState{} : level1_history.back();
    }
    MetaControllerState get_meta() const { return current_meta; }
    bool is_fractal() const { return detector.is_fractal(); }
    double fractal_similarity() const { return detector.last_similarity(); }
    double get_threshold() const { return current_meta.adjusted_threshold; }
    double get_learning_rate() const { return current_meta.learning_rate; }
    uint32_t get_min_level() const { return current_meta.learned_min_level; }
    bool parameters_stable() const { return current_meta.parameters_stable; }
};

// Complete Bootstrap Engine: GF-N seed rotation + recursive fractal controller.
template<int GFNLayers = 5, size_t HistorySize = 30>
class CompleteBootstrap {
private:
    GFNState<GFNLayers> gf_state;
    RecursiveFractalController<HistorySize> controller;
    int bootstrap_count;
    bool initialized;

public:
    CompleteBootstrap() : bootstrap_count(0), initialized(false) {}

    void initialize(double seed) {
        SideChannelDefense::prefault_stack();
        gf_state.initialize(seed);
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
    constexpr int get_total_layers() const { return GFNLayers; }
    double get_min_cassini() const { return initialized ? gf_state.get_min_cassini() : 0.0; }
    int get_bootstrap_count() const { return bootstrap_count; }
    int get_rotation_count() const { return gf_state.get_rotation_count(); }
    bool is_initialized() const { return initialized; }
    bool is_healthy() const { return initialized && gf_state.is_healthy(); }

    ControllerState get_controller_state() const { return controller.get_state(); }
    MetaControllerState get_meta_state() const { return controller.get_meta(); }
    bool is_fractal_converged() const { return controller.is_fractal(); }
    double get_fractal_similarity() const { return controller.fractal_similarity(); }
    double get_learned_threshold() const { return controller.get_threshold(); }
    double get_learning_rate() const { return controller.get_learning_rate(); }
    uint32_t get_min_level() const { return controller.get_min_level(); }
    bool parameters_stable() const { return controller.parameters_stable(); }

    template<typename CipherTextType, typename SecureContextType>
    CipherTextType bootstrap_auto(const CipherTextType& ct, SecureContextType& sc) {
        if (!initialized) return ct;

        SideChannelDefense::force_const_time();

        OperationalState op;
        op.cassini_min = gf_state.get_min_cassini();
        op.healthy_layers = gf_state.verify_and_count();
        op.total_layers = GFNLayers;
        op.ckks_level = ct->GetLevel();
        op.ckks_max_level = 32;
        op.ops_since_bootstrap = bootstrap_count;

        ControllerState ctrl = controller.update(op);

        if (ctrl.should_bootstrap) {
            SideChannelDefense::prefault_stack();
            SideChannelDefense::force_const_time();

            Plaintext temp_pt;
            sc.cc->Decrypt(sc.kp.secretKey, ct, &temp_pt);
            double current_val = temp_pt->GetCKKSPackedValue()[0].real();

            gf_state.rotate_seeds(fabs(current_val));
            bootstrap_count++;

            auto fresh_ct = sc.cc->Encrypt(sc.kp.publicKey,
                sc.cc->MakeCKKSPackedPlaintext(std::vector<double>{current_val}));

            SideChannelDefense::memory_barrier();
            return fresh_ct;
        }

        return ct;
    }
};
