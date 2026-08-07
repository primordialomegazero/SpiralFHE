#include <iostream>
#include <iomanip>
#include <cmath>
#include "src/adaptive/auto_bootstrap.h"

int main() {
    AutoBootstrap ab;
    
    std::cout << "\n╔══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  AUTO BOOTSTRAP v3 — Φ-Integrated Causal State Detection   ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════════╝\n\n";
    
    int passes = 0, fails = 0;
    
    // Test 1: Healthy → STATE_IDLE, Φ near 0
    std::cout << "--- Test 1: Healthy → Φ ≈ 0 → IDLE ---\n";
    for (int i = 0; i < 5; i++) {
        auto s = ab.sense(0.1, 0.9, 0.5);
        std::cout << "  Gate " << i << ": Φ=" << std::fixed << std::setprecision(3) << ab.get_phi() 
                  << " → " << ab.status() << "\n";
    }
    if (ab.current_state == AutoBootstrap::STATE_IDLE) { std::cout << "  ✓ Stays IDLE (low Φ)\n"; passes++; }
    else { std::cout << "  ✗ Wrong state: " << ab.status() << "\n"; fails++; }
    std::cout << "\n";
    
    // Test 2: Rising noise → Φ increases → state progression
    std::cout << "--- Test 2: Φ-Weighted State Progression ---\n";
    AutoBootstrap ab2;
    ab2.gates_until_action = 2;  // Faster evaluation for test
    double noises[] = {0.1, 0.35, 0.55, 0.7, 0.85};
    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 6; j++) ab2.sense(noises[i], 0.6, 0.4);  // Build history
        auto s = ab2.sense(noises[i], 0.6, 0.4);
        std::cout << "  noise=" << noises[i] << " → Φ=" << std::fixed << std::setprecision(3) << ab2.get_phi() 
                  << " → " << ab2.status();
        if (ab2.state_changed()) std::cout << " (transition)";
        std::cout << "\n";
    }
    // After 0.85, should be REFRESH or higher
    if (ab2.current_state >= AutoBootstrap::STATE_REFRESH) { std::cout << "  ✓ Reached REFRESH state\n"; passes++; }
    else { std::cout << "  ✗ Only reached " << ab2.status() << "\n"; fails++; }
    std::cout << "\n";
    
    // Test 3: Critical → EMERGENCY
    std::cout << "--- Test 3: Critical → EMERGENCY ---\n";
    AutoBootstrap ab3;
    for (int i = 0; i < 6; i++) ab3.sense(0.92, 0.08, 0.01);
    std::cout << "  Φ=" << std::fixed << std::setprecision(3) << ab3.get_phi() << " → " << ab3.status() << "\n";
    if (ab3.current_state == AutoBootstrap::STATE_EMERGENCY) { std::cout << "  ✓ Emergency triggered\n"; passes++; }
    else { std::cout << "  ✗ Not emergency\n"; fails++; }
    std::cout << "\n";
    
    // Test 4: Safety gate → FORCE REFRESH
    std::cout << "--- Test 4: Safety Gate Limit → Force Refresh ---\n";
    AutoBootstrap ab4;
    ab4.gates_force_refresh = 12;
    for (int i = 0; i < 13; i++) ab4.sense(0.1, 0.9, 0.5);
    std::cout << "  Gate 13 → " << ab4.status() << "\n";
    if (ab4.should_bootstrap()) { std::cout << "  ✓ Force refresh triggered\n"; passes++; }
    else { std::cout << "  ✗ Should have refreshed\n"; fails++; }
    std::cout << "\n";
    
    // Test 5: Φ-weighted batch reduction
    std::cout << "--- Test 5: Φ-Weighted Batch Optimization ---\n";
    AutoBootstrap ab5;
    ab5.gates_until_action = 2;
    std::cout << "  Initial batch: " << ab5.recommended_batch << "\n";
    for (int i = 0; i < 12; i++) ab5.sense(0.6, 0.4, 0.3);
    std::cout << "  After high-Φ state: " << ab5.recommended_batch << " | Φ=" << std::fixed << std::setprecision(3) << ab5.get_phi() << "\n";
    if (ab5.recommended_batch < 50) { std::cout << "  ✓ Batch reduced via φ-weighting\n"; passes++; }
    else { std::cout << "  ✗ Batch should be reduced\n"; fails++; }
    std::cout << "\n";
    
    // Test 6: Φ-falling → batch increase
    std::cout << "--- Test 6: Φ-Falling → Batch Increase ---\n";
    AutoBootstrap ab6;
    ab6.gates_until_action = 2;
    // batch rebalance auto-managed
    ab6.batch_increase_trigger = 0.55;
    ab6.set_batch(10);
    for (int i = 0; i < 30; i++) ab6.sense(0.1, 0.9, 0.6);
    std::cout << "  After low-Φ recovery: " << ab6.recommended_batch << " (was 10) | Φ=" << std::fixed << std::setprecision(3) << ab6.get_phi() << "\n";
    if (ab6.recommended_batch > 10) { std::cout << "  ✓ Batch increased\n"; passes++; }
    else { std::cout << "  ✗ Batch should increase\n"; fails++; }
    
    std::cout << "\n╔══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  PASSED: " << passes << "/" << (passes+fails) << "                                                ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════════╝\n";
    
    return (fails == 0) ? 0 : 1;
}
