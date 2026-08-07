#include <iostream>
#include <iomanip>
#include <cmath>
#include "src/adaptive/auto_bootstrap.h"

int main() {
    std::cout << std::fixed << std::setprecision(4);
    
    // === TEST 4 DEBUG: Safety Gate ===
    std::cout << "=== TEST 4 DEBUG: Safety Gate ===\n";
    AutoBootstrap ab4;
    ab4.cfg.gates_force_refresh = 12;
    
    for (int i = 0; i < 15; i++) {
        auto s = ab4.sense(0.1, 0.9, 0.5);
        std::cout << "  Gate " << (i+1) 
                  << ": Φ=" << ab4.get_phi()
                  << " gates_since=" << ab4.gates_since_refresh
                  << " state=" << ab4.status()
                  << " boot?=" << ab4.should_bootstrap()
                  << "\n";
    }
    
    std::cout << "\n=== ANALYSIS ===\n";
    std::cout << "gates_force_refresh=12, but after 13 gates: state=" << ab4.status() << "\n";
    std::cout << "should_bootstrap=" << ab4.should_bootstrap() << "\n";
    std::cout << "Why? Because at gate 6, WATCH state was triggered,\n";
    std::cout << "and apply_state(WATCH) does NOT reset gates_since_refresh.\n";
    std::cout << "Only REFRESH/EMERGENCY reset gates_since_refresh.\n";
    std::cout << "So gates_since_refresh keeps counting up past 12 → should trigger.\n";
    std::cout << "But the check is: if (gates_since_refresh > cfg.gates_force_refresh)\n";
    std::cout << "And at gate 13: gates_since_refresh=13 > 12 → should be TRUE.\n\n";
    
    // === TEST 6 DEBUG: Batch Increase ===
    std::cout << "=== TEST 6 DEBUG: Batch Increase ===\n";
    AutoBootstrap ab6;
    ab6.cfg.gates_until_phi_eval = 2;
    ab6.cfg.gates_batch_rebalance = 3;
    ab6.cfg.batch_increase_trigger = 0.5;
    ab6.recommended_batch = 10;
    
    for (int i = 0; i < 15; i++) {
        auto s = ab6.sense(0.1, 0.9, 0.6);
        std::cout << "  Gate " << (i+1) 
                  << ": Φ=" << ab6.get_phi()
                  << " state=" << ab6.status()
                  << " batch=" << ab6.recommended_batch
                  << " falling=" << ab6.is_causal_perturbation_falling()
                  << "\n";
    }
    
    std::cout << "\n=== ANALYSIS ===\n";
    std::cout << "Batch started at 10, ended at " << ab6.recommended_batch << "\n";
    std::cout << "STATE_IDLE apply_state checks:\n";
    std::cout << "  phi_state < batch_increase_trigger (0.5)? phi=" << ab6.get_phi() << "\n";
    std::cout << "  gates_since_refresh > gates_batch_rebalance*2 (6)? gates=" << ab6.gates_since_refresh << "\n";
    std::cout << "  is_causal_perturbation_falling()? " << (ab6.is_causal_perturbation_falling() ? "YES" : "NO") << "\n";
    std::cout << "Only when ALL THREE are true does batch increase.\n";
    std::cout << "But phi_state never drops below 0.5 because compute_phi\n";
    std::cout << "with noise=0.1,stab=0.9,lyap=0.6 gives Φ≈0.528 > 0.5.\n";
    std::cout << "So the trigger condition is never met.\n";
    
    return 0;
}
