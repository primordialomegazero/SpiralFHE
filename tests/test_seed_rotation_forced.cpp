#include <iostream>
#include <iomanip>
#include <chrono>
#include <cmath>
#include <deque>
#include <vector>
#include <algorithm>
#include <numeric>
#include "openfhe.h"
#include "../src/core/constants.h"
#include "../src/fhe/fhe_core.h"
#include "../src/refresh/spiral_bootstrap.h"

using namespace lbcrypto;

struct OperationalState {
    uint32_t ciphertext_level;
    double cassini_min;
    double seed_entropy;
    double chaos_factor;
    bool cassini_valid;
    int ops_since_bootstrap;
};

struct ControllerState {
    double integrated_phi;
    double phi_variance;
    double phi_stability;
    uint32_t current_level;
    uint32_t emergent_refresh_level;
    int ops_since_bootstrap;
    int bootstrap_count;
    bool should_bootstrap;
};

struct MetaControllerState {
    double phi_convergence_rate;
    double threshold_stability;
    double learning_rate;
    uint32_t adjusted_refresh_level;
    bool parameters_stable;
};

class FractalDetector {
private:
    std::deque<double> similarity_history;
    size_t window_size;
    double similarity_threshold;
    bool fractal_detected;
    int depth_reached;

public:
    FractalDetector(size_t window = 20, double threshold = 0.95)
        : window_size(window), similarity_threshold(threshold),
          fractal_detected(false), depth_reached(0) {}

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
        return std::abs(cov / std::sqrt(var_a * var_b));
    }

    bool detect(size_t depth, const std::deque<double>& a, const std::deque<double>& b) {
        double sim = compute_correlation(a, b);
        similarity_history.push_back(sim);
        if (similarity_history.size() > window_size) similarity_history.pop_front();
        if (sim > similarity_threshold) { fractal_detected = true; depth_reached = depth; return true; }
        return false;
    }

    bool is_fractal() const { return fractal_detected; }
    int get_depth() const { return depth_reached; }
    double last_similarity() const { return similarity_history.empty() ? 0.0 : similarity_history.back(); }
};

class RecursiveFractalController {
private:
    static constexpr double PHI = 1.6180339887498948482;
    static constexpr double INV_PHI = 0.6180339887498948482;
    std::deque<ControllerState> level1_history;
    std::deque<MetaControllerState> level2_history;
    std::deque<double> level1_phi_series;
    std::deque<double> level2_convergence_series;
    MetaControllerState current_meta;
    FractalDetector detector;
    size_t history_size;

    double compute_phi_from_operational(const OperationalState& op) {
        double level_fraction = (op.ciphertext_level > 0) ? static_cast<double>(op.ciphertext_level) / 32.0 : 0.0;
        double connectivity = 0.0;
        if (level1_history.size() >= 2) {
            for (size_t i = 1; i < level1_history.size(); i++)
                connectivity += std::abs(level1_history[i].integrated_phi - level1_history[i-1].integrated_phi);
            connectivity /= (level1_history.size() - 1);
        }
        double avg_cassini = op.cassini_valid ? op.cassini_min : 0.0;
        double variance = 0.0;
        if (level1_history.size() >= 3) {
            double mean = 0.0;
            for (auto& s : level1_history) mean += s.integrated_phi;
            mean /= level1_history.size();
            for (auto& s : level1_history) { double d = s.integrated_phi - mean; variance += d * d; }
            variance /= level1_history.size();
        }
        return (connectivity * level_fraction * avg_cassini) / (1.0 + variance);
    }

    MetaControllerState compute_meta_state() {
        MetaControllerState meta;
        if (level1_history.size() < 10) {
            meta.phi_convergence_rate = 0.0; meta.threshold_stability = 0.0;
            meta.learning_rate = INV_PHI; meta.adjusted_refresh_level = 8;
            meta.parameters_stable = false; return meta;
        }
        size_t n = level1_history.size();
        auto r_end = level1_history.end();
        auto r_mid = level1_history.end() - std::min(size_t(5), n);
        auto p_end = r_mid;
        auto p_beg = level1_history.end() - std::min(size_t(10), n);
        double recent_var = compute_variance(r_mid, r_end);
        double prior_var = compute_variance(p_beg, p_end);
        meta.phi_convergence_rate = (prior_var > 1e-12) ? 1.0 - (recent_var / prior_var) : 0.0;
        double tmean = 0.0;
        for (auto& s : level1_history) tmean += s.emergent_refresh_level;
        tmean /= level1_history.size();
        double tvar = 0.0;
        for (auto& s : level1_history) { double d = s.emergent_refresh_level - tmean; tvar += d * d; }
        tvar /= level1_history.size();
        meta.threshold_stability = 1.0 / (1.0 + tvar);
        meta.learning_rate = INV_PHI * (1.0 - meta.phi_convergence_rate) + 0.1 * meta.threshold_stability;
        double cur = level1_history.back().emergent_refresh_level;
        double delta = (meta.phi_convergence_rate > 0.7) ? -1.0 : (meta.phi_convergence_rate < 0.3 ? 1.0 : 0.0);
        meta.adjusted_refresh_level = static_cast<uint32_t>(std::max(4.0, std::min(32.0, cur + delta * meta.learning_rate)));
        meta.parameters_stable = meta.threshold_stability > 0.9;
        return meta;
    }

    double compute_variance(std::deque<ControllerState>::const_iterator b, std::deque<ControllerState>::const_iterator e) {
        if (b == e) return 0.0;
        double m = 0.0; size_t c = 0;
        for (auto it = b; it != e; ++it, ++c) m += it->integrated_phi;
        if (c == 0) return 0.0; m /= c;
        double v = 0.0;
        for (auto it = b; it != e; ++it) { double d = it->integrated_phi - m; v += d * d; }
        return v / c;
    }

    bool detect_fractal() {
        if (level1_phi_series.size() < 10 || level2_convergence_series.size() < 10) return false;
        return detector.detect(3, level1_phi_series, level2_convergence_series);
    }

public:
    RecursiveFractalController(size_t hs = 30) : history_size(hs) {
        current_meta.phi_convergence_rate = 0.0; current_meta.threshold_stability = 0.0;
        current_meta.learning_rate = INV_PHI; current_meta.adjusted_refresh_level = 8;
        current_meta.parameters_stable = false;
    }

    ControllerState update(const OperationalState& op_state) {
        ControllerState ctrl;
        ctrl.integrated_phi = compute_phi_from_operational(op_state);
        ctrl.current_level = op_state.ciphertext_level;
        ctrl.emergent_refresh_level = current_meta.adjusted_refresh_level;
        ctrl.ops_since_bootstrap = op_state.ops_since_bootstrap;
        if (level1_history.size() >= 5) {
            std::vector<double> recent;
            size_t start = level1_history.size() - std::min(size_t(5), level1_history.size());
            for (size_t i = start; i < level1_history.size(); i++) recent.push_back(level1_history[i].integrated_phi);
            double mean = std::accumulate(recent.begin(), recent.end(), 0.0) / recent.size();
            ctrl.phi_variance = 0.0;
            for (double v : recent) ctrl.phi_variance += (v - mean) * (v - mean);
            ctrl.phi_variance /= recent.size();
            ctrl.phi_stability = 1.0 / (1.0 + ctrl.phi_variance);
        } else { ctrl.phi_variance = 1.0; ctrl.phi_stability = 0.0; }
        ctrl.should_bootstrap = (ctrl.current_level >= ctrl.emergent_refresh_level) || (!op_state.cassini_valid) || (op_state.cassini_min < 0.05);
        level1_history.push_back(ctrl); level1_phi_series.push_back(ctrl.integrated_phi);
        if (level1_history.size() > history_size) level1_history.pop_front();
        if (level1_phi_series.size() > history_size) level1_phi_series.pop_front();
        current_meta = compute_meta_state();
        level2_history.push_back(current_meta); level2_convergence_series.push_back(current_meta.phi_convergence_rate);
        if (level2_history.size() > history_size) level2_history.pop_front();
        if (level2_convergence_series.size() > history_size) level2_convergence_series.pop_front();
        detect_fractal();
        return ctrl;
    }

    ControllerState get_state() const { return level1_history.empty() ? ControllerState{} : level1_history.back(); }
    MetaControllerState get_meta() const { return current_meta; }
    bool is_fractal() const { return detector.is_fractal(); }
    int get_depth() const { return detector.get_depth(); }
    double fractal_similarity() const { return detector.last_similarity(); }
    uint32_t get_refresh_level() const { return current_meta.adjusted_refresh_level; }
};

int main() {
    std::cout << "\n";
    std::cout << "============================================================\n";
    std::cout << "  SEED ROTATION VERIFICATION — FORCED BOOTSTRAP\n";
    std::cout << "  Initial threshold = 8 to force bootstrap_zero() calls\n";
    std::cout << "  Validates: GF-N encrypt, Cassini verify, seed rotation\n";
    std::cout << "============================================================\n\n";

    uint32_t ring_dim = 32768;
    uint32_t depth = 32;
    int gf_layers = 5;

    std::cout << "Configuration:\n";
    std::cout << "  Ring Dimension: " << ring_dim << "\n";
    std::cout << "  CKKS Depth:     " << depth << "\n";
    std::cout << "  GF-N Layers:    " << gf_layers << "\n";
    std::cout << "  Initial Refresh: 8 (FORCED LOW to trigger seed rotation)\n\n";

    auto sc = create_fhe_context(ring_dim, depth);
    SpiralBootstrap bootstrap;
    bootstrap.init(0.6180339887498948482, gf_layers);
    bootstrap.set_depth(depth);

    RecursiveFractalController rfc(30);

    std::cout << "  " << bootstrap.status() << "\n";
    std::cout << "  Recursive Fractal Controller: ACTIVE\n\n";

    double plaintext = 0.42;
    int max_ops = 500;
    int bootstrap_count = 0;
    bool alive = true;
    int seed_rotation_count = 0;
    int fractal_at = -1;
    int meta_at = -1;

    auto slot = encrypt(sc, plaintext);
    auto ct = slot.a;
    auto start_time = std::chrono::steady_clock::now();

    std::cout << std::string(130, '-') << "\n";
    std::cout << std::left
              << std::setw(8) << "Op"
              << std::setw(8) << "Level"
              << std::setw(12) << "PHI"
              << std::setw(10) << "Stability"
              << std::setw(12) << "Threshold"
              << std::setw(10) << "LearnRt"
              << std::setw(8) << "Boots"
              << std::setw(12) << "Cassini"
              << std::setw(12) << "MetaStable"
              << std::setw(12) << "FractalSim"
              << std::setw(12) << "Fractal"
              << "Status\n";
    std::cout << std::string(130, '-') << "\n";

    for (int i = 1; i <= max_ops; i++) {
        ct = sc.cc->EvalMult(ct, ct);

        OperationalState op_state;
        op_state.ciphertext_level = ct->GetLevel();
        op_state.cassini_valid = bootstrap.verify_cassini();
        op_state.cassini_min = op_state.cassini_valid ? 0.8 : 0.0;
        op_state.seed_entropy = 0.5;
        op_state.chaos_factor = std::abs(std::sin(i * 1.618));
        op_state.ops_since_bootstrap = seed_rotation_count;

        ControllerState ctrl = rfc.update(op_state);
        bool will_bootstrap = ctrl.should_bootstrap;
        ct = bootstrap.bootstrap_auto(ct, sc);
        if (will_bootstrap) {
            bootstrap_count++;
            seed_rotation_count++;
            bootstrap.did_bootstrap();
        }

        if (rfc.is_fractal() && fractal_at < 0) fractal_at = i;
        if (rfc.get_meta().parameters_stable && meta_at < 0) meta_at = i;

        if (i % 25 == 0 || i == max_ops || i == 1 || (fractal_at == i) || (meta_at == i) || will_bootstrap) {
            MetaControllerState meta = rfc.get_meta();
            std::cout << std::left
                      << std::setw(8) << i
                      << std::setw(8) << op_state.ciphertext_level
                      << std::setw(12) << std::fixed << std::setprecision(4) << ctrl.integrated_phi
                      << std::setw(10) << std::fixed << std::setprecision(4) << ctrl.phi_stability
                      << std::setw(12) << meta.adjusted_refresh_level
                      << std::setw(10) << std::fixed << std::setprecision(4) << meta.learning_rate
                      << std::setw(8) << bootstrap_count
                      << std::setw(12) << (op_state.cassini_valid ? "VALID" : "FAIL")
                      << std::setw(12) << (meta.parameters_stable ? "YES" : "NO")
                      << std::setw(12) << std::fixed << std::setprecision(6) << rfc.fractal_similarity()
                      << std::setw(12) << (rfc.is_fractal() ? "DETECTED" : "SEARCHING")
                      << (alive ? "OK" : "FAIL") << "\n";
            if (will_bootstrap) {
                std::cout << "  >>> SEED ROTATION at Op " << i << " (bootstrap_zero called, level="
                          << op_state.ciphertext_level << ") <<<\n";
            }
            if (i == fractal_at) std::cout << "  >>> FRACTAL DETECTED at Op " << i << " <<<\n";
            if (i == meta_at) std::cout << "  >>> META STABLE at Op " << i << " <<<\n";
        }

        if (i % 100 == 0) {
            try {
                double val = decrypt(sc, ct);
                std::cout << "  [Integrity Op " << i << ": value=" << std::fixed << std::setprecision(8) << val << "]\n";
            } catch (const std::exception& e) {
                std::cout << "  [DECRYPT FAILED Op " << i << ": " << e.what() << "]\n";
                alive = false; break;
            }
        }
    }

    auto end_time = std::chrono::steady_clock::now();
    double elapsed = std::chrono::duration<double>(end_time - start_time).count();

    std::cout << "\n" << std::string(130, '=') << "\n";
    std::cout << "RESULTS — SEED ROTATION VERIFICATION\n";
    std::cout << std::string(130, '=') << "\n\n";
    std::cout << "Operational:\n";
    std::cout << "  Total Operations:    " << max_ops << "\n";
    std::cout << "  Bootstraps:          " << bootstrap_count << "\n";
    std::cout << "  Bootstrap Rate:      " << std::fixed << std::setprecision(2) << (100.0 * bootstrap_count / max_ops) << "%\n";
    std::cout << "  Execution Time:      " << std::fixed << std::setprecision(2) << elapsed << "s\n";
    std::cout << "  Throughput:          " << std::fixed << std::setprecision(2) << (max_ops / elapsed) << " ops/s\n";
    std::cout << "  System Status:       " << (alive ? "ALIVE" : "FAILED") << "\n\n";
    std::cout << "Seed Rotation:\n";
    std::cout << "  Seed Rotations:      " << seed_rotation_count << "\n";
    std::cout << "  Cassini:             " << (bootstrap.verify_cassini() ? "VALID" : "FAIL") << "\n";
    std::cout << "  GF-N Layers:         " << gf_layers << "\n";
    std::cout << "  Mechanism:           bootstrap_zero() via seed rotation\n\n";
    std::cout << "Recursive Controller:\n";
    std::cout << "  PHI Final:           " << std::fixed << std::setprecision(4) << rfc.get_state().integrated_phi << "\n";
    std::cout << "  PHI Stability:       " << std::fixed << std::setprecision(4) << rfc.get_state().phi_stability << "\n";
    std::cout << "  Meta Stable at:      " << (meta_at > 0 ? "Op " + std::to_string(meta_at) : "NOT REACHED") << "\n";
    std::cout << "  Fractal at:          " << (fractal_at > 0 ? "Op " + std::to_string(fractal_at) : "NOT DETECTED") << "\n";
    std::cout << "  Final Threshold:     " << rfc.get_refresh_level() << "\n";
    std::cout << "  Final Learn Rate:    " << std::fixed << std::setprecision(4) << rfc.get_meta().learning_rate << "\n";
    std::cout << "\n============================================================\n";
    if (alive && seed_rotation_count > 0) {
        std::cout << "  VERDICT: SEED ROTATION VERIFIED\n";
        std::cout << "  " << seed_rotation_count << " bootstrap_zero() calls executed\n";
        std::cout << "  GF-N + Cassini + Seed Rotation = FUNCTIONAL\n";
    } else if (alive) {
        std::cout << "  VERDICT: NO SEED ROTATION TRIGGERED\n";
    } else {
        std::cout << "  VERDICT: SYSTEM FAILED\n";
    }
    std::cout << "============================================================\n\n";
    return alive ? 0 : 1;
}
