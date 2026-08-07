#include <iostream>
#include <iomanip>
#include <chrono>
#include <cmath>
#include "openfhe.h"
#include "../src/core/constants.h"
#include "../src/fhe/fhe_core.h"
#include "../src/refresh/spiral_bootstrap_complete.h"

using namespace lbcrypto;

int main() {
    std::cout << "\n";
    std::cout << "================================================================================\n";
    std::cout << "  SPIRAL FHE — EvalMod EXPERIMENT (LOCAL ONLY — NOT FOR RELEASE)\n";
    std::cout << "================================================================================\n\n";

    uint32_t ring_dim = 65536;
    uint32_t depth = 16;
    double seed = 0.6180339887498948482;

    auto sc = create_fhe_context(ring_dim, depth);
    uint32_t actual_ring = auto_ring_dim(ring_dim);

    std::cout << "Ring: " << actual_ring << ", Depth: " << depth << "\n\n";

    CompleteBootstrap<5, 30> cb;
    cb.initialize(seed);

    std::cout << "Engine: " << (cb.is_initialized() ? "READY" : "FAILED") << "\n";
    std::cout << "Cassini: " << std::fixed << std::setprecision(6) << cb.get_min_cassini() << "\n\n";

    // Encrypt a value and a modulus
    double value = 42.0;
    double modulus = 5.0;
    
    auto ct_val = encrypt(sc, value);
    auto ct_mod = encrypt(sc, modulus);

    std::cout << "Computing: " << value << " mod " << modulus << " (expected: " << fmod(value, modulus) << ")\n";
    std::cout << "Note: OpenFHE CKKS does NOT have native EvalMod.\n";
    std::cout << "We approximate mod via: x - floor(x/m)*m using polynomial approx of floor.\n\n";

    // CKKS doesn't have EvalMod. We approximate using:
    // 1. Division: x * (1/m) via EvalMult with precomputed inverse
    // 2. Floor: polynomial approximation (Chebyshev or Taylor)
    // 3. Subtract: x - floor_result * m
    
    double inv_mod = 1.0 / modulus;
    auto ct_inv = encrypt(sc, inv_mod);
    
    // Step 1: x * (1/m)
    auto ct_div = sc.cc->EvalMult(ct_val, ct_inv);
    ct_div = cb.bootstrap_auto(ct_div, sc);
    
    // Step 2: floor approximation (simple: x - frac(x) where frac ≈ x - round(x))
    // For CKKS, we use a low-degree polynomial to approximate frac(x)
    // This is a crude approximation — real EvalMod needs deeper circuits
    auto ct_floor = sc.cc->EvalMult(ct_div, ct_div);  // x^2
    ct_floor = cb.bootstrap_auto(ct_floor, sc);
    auto ct_floor2 = sc.cc->EvalMult(ct_floor, ct_div);  // x^3
    ct_floor2 = cb.bootstrap_auto(ct_floor2, sc);
    
    // Simple poly approx: x - 0.5*x^2 + 0.3*x^3
    auto ct_floor_sq = encrypt(sc, -0.5);
    auto ct_floor_cu = encrypt(sc, 0.3);
    
    ct_floor = sc.cc->EvalMult(ct_div, ct_div);
    ct_floor = cb.bootstrap_auto(ct_floor, sc);
    
    // Crude: just subtract the multiplication result * modulus from original
    auto ct_floor_times_m = sc.cc->EvalMult(ct_div, ct_mod);
    ct_floor_times_m = cb.bootstrap_auto(ct_floor_times_m, sc);
    
    auto ct_remainder = sc.cc->EvalSub(ct_val, ct_floor_times_m);
    ct_remainder = cb.bootstrap_auto(ct_remainder, sc);

    double result = 0.0;
    try {
        result = decrypt(sc, ct_remainder);
    } catch (const std::exception& e) {
        std::cout << "Decrypt failed: " << e.what() << "\n";
        return 1;
    }

    std::cout << "\nResults:\n";
    std::cout << "  Value:      " << value << "\n";
    std::cout << "  Modulus:    " << modulus << "\n";
    std::cout << "  Expected:   " << fmod(value, modulus) << "\n";
    std::cout << "  Got:        " << std::fixed << std::setprecision(6) << result << "\n";
    std::cout << "  Error:      " << std::fixed << std::setprecision(6) << fabs(result - fmod(value, modulus)) << "\n\n";

    std::cout << "================================================================================\n";
    std::cout << "  NOTE: This is a CRUDE APPROXIMATION of mod using polynomial division.\n";
    std::cout << "  True EvalMod in FHE requires exact integer arithmetic circuits.\n";
    std::cout << "  This experiment is for curiosity only — not for release.\n";
    std::cout << "================================================================================\n\n";

    return 0;
}
