#include <iostream>
#include <iomanip>
#include <cmath>
#include "src/adaptive/auto_bootstrap.h"

int main() {
    std::cout << std::fixed << std::setprecision(4);
    
    AutoBootstrap ab;
    ab.cfg.gates_until_phi_eval = 2;
    ab.cfg.gates_batch_rebalance = 3;
    ab.cfg.batch_increase_trigger = 0.618;  // Starting value
    ab.cfg.tuning_window = 8;  // Faster tuning for test
    ab.recommended_batch = 10;
    
    std::cout << "=== DEEP DEBUG: Batch Increase ===\n";
    std::cout << "Initial: batch=" << ab.recommended_batch 
              << " batch_inc_trigger=" << ab.cfg.batch_increase_trigger << "\n\n";
    
    for (int i = 0; i < 20; i++) {
        auto s = ab.sense(0.1, 0.9, 0.6);
        std::cout << "Gate " << std::setw(2) << (i+1) 
                  << ": Φ=" << ab.get_phi()
                  << " Φ_avg=" << ab.get_phi_avg()
                  << " state=" << std::setw(8) << ab.status()
                  << " batch=" << std::setw(3) << ab.recommended_batch
                  << " trigger=" << ab.cfg.batch_increase_trigger
                  << " falling=" << ab.is_causal_perturbation_falling()
                  << " g_since=" << ab.gates_since_refresh
                  << "\n";
        
        if (ab.state_changed()) {
            std::cout << "  → TRANSITION to " << ab.status() << "\n";
        }
    }
    
    std::cout << "\n=== ANALYSIS ===\n";
    std::cout << "The system NEVER enters STATE_IDLE after gate 6.\n";
    std::cout << "Why? Φ=0.528 > noise_watch(0.382) → always WATCH.\n";
    std::cout << "STATE_IDLE apply_state has batch increase logic.\n";
    std::cout << "But STATE_WATCH apply_state only has batch REDUCTION.\n";
    std::cout << "So batch can only go DOWN, never UP.\n\n";
    std::cout << "ROOT CAUSE: The Φ floor for healthy metrics (0.1,0.9,0.6)\n";
    std::cout << "is 0.528, which is above the WATCH threshold (0.382).\n";
    std::cout << "So the system is stuck in WATCH permanently.\n";
    std::cout << "Auto-tune should EVENTUALLY raise noise_watch above 0.528.\n";
    std::cout << "But tuning_window=8 is too slow for 20 gates.\n\n";
    
    // Let it run longer to see if auto-tune kicks in
    std::cout << "=== RUNNING 100 MORE GATES ===\n";
    for (int i = 0; i < 100; i++) {
        ab.sense(0.1, 0.9, 0.6);
    }
    std::cout << "After 120 gates:\n";
    std::cout << "  Φ_avg=" << ab.get_phi_avg() << "\n";
    std::cout << "  noise_watch=" << ab.cfg.noise_watch << " (was 0.382)\n";
    std::cout << "  batch_inc_trigger=" << ab.cfg.batch_increase_trigger << " (was 0.618)\n";
    std::cout << "  state=" << ab.status() << "\n";
    std::cout << "  batch=" << ab.recommended_batch << "\n";
    
    if (ab.cfg.noise_watch > 0.5) {
        std::cout << "  ✓ Auto-tune RAISED noise_watch above Φ floor\n";
    } else {
        std::cout << "  ✗ Auto-tune did not raise noise_watch enough\n";
    }
    
    return 0;
}
