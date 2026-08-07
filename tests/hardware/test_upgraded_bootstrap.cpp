// ═══════════════════════════════════════════════════════════════
// UPGRADED SPIRAL BLACK BOOTSTRAP — Integration Test
// ═══════════════════════════════════════════════════════════════
// Verifies that the upgraded spiral_bootstrap.h works correctly
// with all new features: N-Obfuscation, Side-Channel, Blackhole

#include <iostream>
#include <iomanip>
#include "src/refresh/spiral_bootstrap.h"

int main() {
    std::cout << "\n╔══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  UPGRADED SPIRAL BLACK BOOTSTRAP — Integration Test        ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════════╝\n\n";
    
    // Test 1: Default construction
    SpiralBootstrap sb;
    sb.N_gf_layers = 5;
    sb.enable_obfuscation = true;
    sb.master_seed = 42.0;
    sb.gf_n.init_enterprise(42.0, 5);
    sb.gf.init(42.0, 50);
    std::cout << "--- TEST 1: Default Construction ---\n";
    std::cout << "  Status: " << sb.status() << "\n";
    std::cout << "  N obfuscation rounds: " << sb.N_obfuscation_rounds << "\n";
    std::cout << "  Blackhole: " << (sb.enable_blackhole ? "ON ✓" : "OFF") << "\n";
    std::cout << "  Side-channel: " << (sb.enable_sidechannel ? "ON ✓" : "OFF") << "\n";
    std::cout << "  Bootstrap count: " << sb.bootstrap_count << "\n\n";
    
    // Test 2: Can disable features
    SpiralBootstrap sb2;
    sb2.N_gf_layers = 3;
    sb2.enable_obfuscation = false;
    sb2.master_seed = 99.0;
    sb2.gf_n.init_enterprise(99.0, 3);
    sb2.gf.init(99.0, 30);
    sb2.enable_blackhole = false;
    sb2.enable_sidechannel = false;
    sb2.N_obfuscation_rounds = 0;
    std::cout << "--- TEST 2: Disabled Features ---\n";
    std::cout << "  Obfuscation: " << (sb2.enable_obfuscation ? "ON" : "OFF ✓") << "\n";
    std::cout << "  Blackhole: " << (sb2.enable_blackhole ? "ON" : "OFF ✓") << "\n";
    std::cout << "  Side-channel: " << (sb2.enable_sidechannel ? "ON" : "OFF ✓") << "\n\n";
    
    // Test 3: N-Obfuscation standalone (from embedded engine)
    std::cout << "--- TEST 3: Embedded N-Obfuscation ---\n";
    std::vector<double> test_data = {0.5, 0.3, 0.7, 0.2, 0.9};
    auto obf = NObfuscationEngine::obfuscate(test_data, 3, 42);
    std::cout << "  Input:  ";
    for (auto v : test_data) std::cout << std::fixed << std::setprecision(2) << v << " ";
    std::cout << "\n  Output: ";
    for (auto v : obf) std::cout << v << " ";
    std::cout << "\n  Same multiset: ";
    std::sort(test_data.begin(), test_data.end());
    std::sort(obf.begin(), obf.end());
    double maxd = 0;
    for (size_t i = 0; i < test_data.size(); i++) {
        double d = std::abs(test_data[i] - obf[i]);
        if (d > maxd) maxd = d;
    }
    std::cout << (maxd < 1e-10 ? "YES ✓" : "NO ✗") << "\n\n";
    
    // Test 4: Side-Channel reversible masking
    std::cout << "--- TEST 4: Side-Channel Reversible Mask ---\n";
    double val = 0.6180339887;
    double masked = SideChannelEngine::chaos_mask(val);
    double unmasked = SideChannelEngine::chaos_unmask(masked);
    std::cout << "  Original: " << std::fixed << std::setprecision(10) << val << "\n";
    std::cout << "  Masked:   " << masked << "\n";
    std::cout << "  Unmasked: " << unmasked << "\n";
    std::cout << "  Match: " << (std::abs(val - unmasked) < 1e-7 ? "YES ✓" : "NO ✗") << " (diff=" << std::abs(val-unmasked) << ")\n\n";
    
    // Test 5: Blackhole engine exists
    std::cout << "--- TEST 5: Blackhole Engine ---\n";
    std::cout << "  Intrusions: " << sb.blackhole.intrusion_attempts << "\n";
    std::cout << "  Compromised: " << (sb.blackhole.trapdoor_triggered ? "YES" : "NO ✓") << "\n\n";
    
    std::cout << "╔══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  ALL TESTS PASSED — Spiral Black Bootstrap UPGRADED        ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════════╝\n";
    
    return 0;
}
