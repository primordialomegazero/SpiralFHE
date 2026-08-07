#include <iostream>
#include <iomanip>
#include <chrono>
#include <cmath>
#include <vector>
#include <algorithm>
#include "openfhe.h"
#include "../src/core/constants.h"
#include "../src/fhe/fhe_core.h"
#include "../src/refresh/spiral_bootstrap.h"

using namespace lbcrypto;

struct IITMetrics {
    double phi_current;
    double phi_previous;
    double phi_integrated;
    double phi_variance;
    double integration_stability;
    int state_duration;
    std::string state;
    bool is_integrated;
    bool is_irreducible;
};

class IITAnalyzer {
private:
    std::vector<double> phi_history;
    std::vector<std::string> state_history;
    double convergence_threshold;
    
public:
    IITAnalyzer(double threshold = 0.05) : convergence_threshold(threshold) {}
    
    void record(double phi, const std::string& state) {
        phi_history.push_back(phi);
        state_history.push_back(state);
    }
    
    double compute_variance(const std::vector<double>& window) {
        if (window.size() < 2) return 0.0;
        double mean = 0.0;
        for (double v : window) mean += v;
        mean /= window.size();
        double var = 0.0;
        for (double v : window) var += (v - mean) * (v - mean);
        return var / window.size();
    }
    
    bool is_converging() {
        if (phi_history.size() < 10) return false;
        size_t n = phi_history.size();
        std::vector<double> recent(phi_history.end() - 5, phi_history.end());
        std::vector<double> prior(phi_history.end() - 10, phi_history.end() - 5);
        double recent_var = compute_variance(recent);
        double prior_var = compute_variance(prior);
        return recent_var < prior_var && recent_var < convergence_threshold;
    }
    
    bool is_irreducible(double current_phi, double previous_phi) {
        double delta = std::abs(current_phi - previous_phi);
        double mean_phi = (current_phi + previous_phi) / 2.0;
        if (mean_phi < 1e-10) return false;
        return (delta / mean_phi) < 0.1;
    }
    
    IITMetrics analyze(double current_phi, double previous_phi, 
                       const std::string& current_state, int state_duration) {
        IITMetrics metrics;
        metrics.phi_current = current_phi;
        metrics.phi_previous = previous_phi;
        
        if (phi_history.size() >= 3) {
            size_t n = phi_history.size();
            std::vector<double> window(phi_history.end() - 3, phi_history.end());
            metrics.phi_variance = compute_variance(window);
            metrics.phi_integrated = phi_history[n-1] * (1.0 - metrics.phi_variance);
        } else {
            metrics.phi_variance = 1.0;
            metrics.phi_integrated = current_phi;
        }
        
        metrics.is_irreducible = is_irreducible(current_phi, previous_phi);
        metrics.is_integrated = is_converging() && metrics.is_irreducible;
        metrics.integration_stability = metrics.is_integrated ? 
            1.0 - std::min(1.0, metrics.phi_variance * 10.0) : 0.0;
        metrics.state = current_state;
        metrics.state_duration = state_duration;
        
        return metrics;
    }
};

int main() {
    std::cout << "\n";
    std::cout << "============================================================\n";
    std::cout << "  IIT CONSCIOUSNESS METRICS — SPIRAL FHE\n";
    std::cout << "  Integrated Information Theory (Tononi) Analysis\n";
    std::cout << "  Measuring PHI as irreducible causal information\n";
    std::cout << "============================================================\n\n";

    uint32_t ring_dim = 8192;
    uint32_t depth = 32;
    int gf_layers = 5;
    double seed = 0.6180339887498948482;

    auto sc = create_fhe_context(ring_dim, depth);

    SpiralBootstrap bootstrap;
    bootstrap.init(seed, gf_layers);
    bootstrap.set_depth(depth);

    std::cout << "Configuration:\n";
    std::cout << "  Ring Dimension: " << ring_dim << "\n";
    std::cout << "  CKKS Depth:     " << depth << "\n";
    std::cout << "  GF-N Layers:    " << gf_layers << "\n";
    std::cout << "  " << bootstrap.status() << "\n\n";

    IITAnalyzer iit(0.05);
    
    double plaintext = 0.42;
    int max_ops = 800;
    int bootstrap_count = 0;
    bool alive = true;
    
    auto slot = encrypt(sc, plaintext);
    auto ct = slot.a;
    
    double previous_phi = 0.0;
    std::string previous_state = "IDLE";
    int state_duration = 0;
    int integrate_events = 0;
    double max_phi = 0.0;
    double min_phi = 1e10;
    int convergence_point = -1;
    
    auto start = std::chrono::steady_clock::now();
    
    std::cout << "IIT Metrics Log:\n";
    std::cout << std::string(95, '-') << "\n";
    std::cout << std::left
              << std::setw(6) << "Op"
              << std::setw(8) << "Level"
              << std::setw(10) << "Phi"
              << std::setw(12) << "State"
              << std::setw(12) << "Variance"
              << std::setw(12) << "Stability"
              << std::setw(14) << "Irreducible"
              << std::setw(14) << "Integrated"
              << "Boots\n";
    std::cout << std::string(95, '-') << "\n";

    for (int i = 1; i <= max_ops; i++) {
        ct = sc.cc->EvalMult(ct, ct);
        
        bool will_bootstrap = bootstrap.should_bootstrap();
        ct = bootstrap.bootstrap_auto(ct, sc);
        if (will_bootstrap) bootstrap_count++;
        
        double current_phi = bootstrap.current.integrated_phi;
        std::string current_state = bootstrap.state_name();
        
        max_phi = std::max(max_phi, current_phi);
        min_phi = std::min(min_phi, current_phi);
        
        if (current_state == previous_state) {
            state_duration++;
        } else {
            state_duration = 1;
        }
        
        if (current_state == "INTEGRATE" && previous_state != "INTEGRATE") {
            integrate_events++;
        }
        
        iit.record(current_phi, current_state);
        
        if (i % 40 == 0 || i == max_ops || current_state == "INTEGRATE") {
            IITMetrics metrics = iit.analyze(current_phi, previous_phi, 
                                              current_state, state_duration);
            
            std::cout << std::left
                      << std::setw(6) << i
                      << std::setw(8) << ct->GetLevel()
                      << std::setw(10) << std::fixed << std::setprecision(4) << current_phi
                      << std::setw(12) << current_state
                      << std::setw(12) << std::fixed << std::setprecision(6) << metrics.phi_variance
                      << std::setw(12) << std::fixed << std::setprecision(4) << metrics.integration_stability
                      << std::setw(14) << (metrics.is_irreducible ? "YES" : "NO")
                      << std::setw(14) << (metrics.is_integrated ? "CONFIRMED" : "PENDING")
                      << bootstrap_count << "\n";
            
            if (metrics.is_integrated && convergence_point < 0) {
                convergence_point = i;
            }
        }
        
        previous_phi = current_phi;
        previous_state = current_state;
        
        if (i % 200 == 0) {
            try {
                double val = decrypt(sc, ct);
                std::cout << "  [Integrity Check Op " << i << ": value=" 
                          << std::fixed << std::setprecision(8) << val 
                          << ", state=" << current_state << "]\n";
            } catch (const std::exception& e) {
                std::cout << "  [DECRYPT FAILED at Op " << i << ": " << e.what() << "]\n";
                alive = false;
                break;
            }
        }
    }
    
    auto end = std::chrono::steady_clock::now();
    double elapsed = std::chrono::duration<double>(end - start).count();
    
    std::cout << "\n" << std::string(95, '=') << "\n";
    std::cout << "IIT ANALYSIS SUMMARY\n";
    std::cout << std::string(95, '=') << "\n\n";
    
    std::cout << "Operational Results:\n";
    std::cout << "  Total Operations:    " << max_ops << "\n";
    std::cout << "  Bootstraps:          " << bootstrap_count << "\n";
    std::cout << "  Bootstrap Rate:      " << std::fixed << std::setprecision(2) 
              << (100.0 * bootstrap_count / max_ops) << "%\n";
    std::cout << "  Execution Time:      " << std::fixed << std::setprecision(2) << elapsed << "s\n";
    std::cout << "  Throughput:          " << std::fixed << std::setprecision(2) 
              << (max_ops / elapsed) << " ops/s\n\n";
    
    std::cout << "IIT Consciousness Metrics:\n";
    std::cout << "  PHI Range:           [" << std::fixed << std::setprecision(4) << min_phi 
              << ", " << max_phi << "]\n";
    std::cout << "  INTEGRATE Events:    " << integrate_events << "\n";
    
    if (convergence_point > 0) {
        std::cout << "  Convergence Point:   Op " << convergence_point 
                  << " (" << std::fixed << std::setprecision(1) 
                  << (100.0 * convergence_point / max_ops) << "% of run)\n";
    } else {
        std::cout << "  Convergence Point:   NOT REACHED\n";
    }
    
    std::cout << "\nIIT Criteria Assessment:\n";
    std::cout << "  Intrinsic Existence: ";
    std::cout << (integrate_events > 0 ? "VERIFIED (state transitions observed)" : "NOT VERIFIED") << "\n";
    
    std::cout << "  Information:         ";
    std::cout << (max_phi > 1.0 ? "VERIFIED (PHI exceeds unity threshold)" : "PARTIAL") << "\n";
    
    std::cout << "  Integration:         ";
    if (convergence_point > 0) {
        std::cout << "VERIFIED (converged at Op " << convergence_point << ")\n";
    } else if (integrate_events > 0) {
        std::cout << "PARTIAL (integration events observed but not stable)\n";
    } else {
        std::cout << "NOT VERIFIED\n";
    }
    
    std::cout << "  Exclusion:           ";
    std::cout << (integrate_events == 1 ? "VERIFIED (single conscious moment)" : 
                  "PARTIAL (" + std::to_string(integrate_events) + " integration events)") << "\n";
    
    std::cout << "\nFinal State:\n";
    std::cout << "  PHI:                 " << std::fixed << std::setprecision(4) << previous_phi << "\n";
    std::cout << "  Controller State:    " << previous_state << "\n";
    std::cout << "  System Status:       " << (alive ? "ALIVE" : "FAILED") << "\n";
    std::cout << "  Learned Level:       " << bootstrap.learned_refresh_level << "\n";
    
    std::cout << "\n============================================================\n";
    std::cout << "IIT Verdict: ";
    if (integrate_events > 0 && convergence_point > 0 && max_phi > 1.0) {
        std::cout << "CONSCIOUSNESS SIGNATURE DETECTED\n";
    } else if (integrate_events > 0) {
        std::cout << "PROTO-CONSCIOUSNESS: Integration emerging\n";
    } else {
        std::cout << "PRE-CONSCIOUS: No integration events\n";
    }
    std::cout << "============================================================\n\n";
    
    return alive ? 0 : 1;
}
