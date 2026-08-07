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
    std::cout << "  SPIRAL FHE — VALUE PRESERVATION TEST — 64K RING\n";
    std::cout << "  Self-Optimizing Bootstrap + GF-N Seed Rotation + Cassini Verification\n";
    std::cout << "================================================================================\n\n";

    uint32_t ring_dim = 65536;
    uint32_t depth = 16;
    int gf_layers = 5;
    double seed = 0.6180339887498948482;

    auto sc = create_fhe_context(ring_dim, depth);
    CompleteBootstrap<5, 30> cb;
    cb.initialize(seed);

    std::cout << "Configuration:\n";
    std::cout << "  Ring Dimension:      " << ring_dim << "\n";
    std::cout << "  CKKS Depth:          " << depth << "\n";
    std::cout << "  GF-N Layers:         " << gf_layers << "\n";
    std::cout << "  Controller:          Recursive Fractal (3-level self-optimizing)\n";
    std::cout << "  History Size:        30\n";
    std::cout << "  PHI Signal:          Cassini health (not CKKS level)\n";
    std::cout << "  Bootstrap Decision:  Cassini threshold (not modulus level)\n\n";

    std::cout << "Initial State:\n";
    std::cout << "  Engine:              " << (cb.is_initialized() ? "READY" : "FAILED") << "\n";
    std::cout << "  Integrity:           " << (cb.verify_integrity() ? "PASS" : "FAIL") << "\n";
    std::cout << "  Healthy Layers:      " << cb.get_healthy_layers() << "/" << gf_layers << "\n";
    std::cout << "  Min Cassini:         " << std::fixed << std::setprecision(6) << cb.get_min_cassini() << "\n\n";

    double original_value = 24325.0;
    double add_amount = 1000.0;
    double sub_amount = 1000.0;
    int cycles = 100;

    auto ct_original = encrypt(sc, original_value);
    auto ct_add = encrypt(sc, add_amount);
    auto ct_sub = encrypt(sc, sub_amount);

    auto ct = ct_original.a;
    auto ct_one = ct_add.a;
    auto ct_one_neg = ct_sub.a;

    bool alive = true;
    int bootstrap_count = 0;
    int last_bootstrap = 0;
    int meta_stable_at = -1;
    int fractal_at = -1;

    auto start_time = std::chrono::steady_clock::now();

    std::cout << "Operation: ((value + " << add_amount << ") - " << sub_amount << ") repeated " << cycles << " times\n";
    std::cout << "Expected final value: " << original_value << "\n\n";

    std::cout << std::string(125, '-') << "\n";
    std::cout << std::left
              << std::setw(8) << "Cycle"
              << std::setw(18) << "Decrypted"
              << std::setw(14) << "Error"
              << std::setw(14) << "CassiniMin"
              << std::setw(14) << "Threshold"
              << std::setw(10) << "LearnRt"
              << std::setw(10) << "Healthy"
              << std::setw(8) << "Boots"
              << std::setw(12) << "MetaStable"
              << std::setw(12) << "Fractal"
              << "Status\n";
    std::cout << std::string(125, '-') << "\n";

    for (int i = 1; i <= cycles; i++) {
        ct = sc.cc->EvalAdd(ct, ct_one);
        ct = cb.bootstrap_auto(ct, sc);

        ct = sc.cc->EvalSub(ct, ct_one_neg);
        ct = cb.bootstrap_auto(ct, sc);

        int current_boots = cb.get_bootstrap_count();
        if (current_boots > last_bootstrap) {
            bootstrap_count += (current_boots - last_bootstrap);
            last_bootstrap = current_boots;
        }

        if (cb.is_fractal_converged() && fractal_at < 0) fractal_at = i;
        if (cb.parameters_stable() && meta_stable_at < 0) meta_stable_at = i;

        if (i % 10 == 0 || i == 1 || i == cycles || i == fractal_at || i == meta_stable_at) {
            auto now = std::chrono::steady_clock::now();
            double elapsed = std::chrono::duration<double>(now - start_time).count();

            double val = 0.0;
            bool decrypt_ok = true;
            try {
                Plaintext pt;
                sc.cc->Decrypt(sc.kp.secretKey, ct, &pt);
                val = pt->GetCKKSPackedValue()[0].real();
            } catch (const std::exception& e) {
                decrypt_ok = false;
                alive = false;
            }

            double error = fabs(val - original_value);
            MetaControllerState meta = cb.get_meta_state();
            ControllerState ctrl = cb.get_controller_state();

            std::cout << std::left
                      << std::setw(8) << i
                      << std::setw(18) << std::fixed << std::setprecision(2) << val
                      << std::setw(14) << std::fixed << std::setprecision(6) << error
                      << std::setw(14) << std::fixed << std::setprecision(6) << cb.get_min_cassini()
                      << std::setw(14) << std::fixed << std::setprecision(4) << meta.adjusted_threshold
                      << std::setw(10) << std::fixed << std::setprecision(4) << meta.learning_rate
                      << std::setw(10) << (std::to_string(ctrl.healthy_layers) + "/" + std::to_string(gf_layers))
                      << std::setw(8) << cb.get_bootstrap_count()
                      << std::setw(12) << (meta.parameters_stable ? "YES" : "NO")
                      << std::setw(12) << (cb.is_fractal_converged() ? "DETECTED" : "SEARCHING")
                      << (decrypt_ok ? "OK" : "FAIL") << "\n";

            if (i == fractal_at) std::cout << "  >>> FRACTAL CONVERGENCE at Cycle " << i << " <<<\n";
            if (i == meta_stable_at) std::cout << "  >>> META PARAMETERS STABLE at Cycle " << i << " <<<\n";
            if (!decrypt_ok) break;
        }
    }

    auto end_time = std::chrono::steady_clock::now();
    double total_elapsed = std::chrono::duration<double>(end_time - start_time).count();

    double final_val = 0.0;
    bool final_ok = true;
    try {
        Plaintext pt;
        sc.cc->Decrypt(sc.kp.secretKey, ct, &pt);
        final_val = pt->GetCKKSPackedValue()[0].real();
    } catch (const std::exception& e) {
        final_ok = false;
    }

    std::cout << "\n" << std::string(125, '=') << "\n";
    std::cout << "RESULTS — VALUE PRESERVATION TEST\n";
    std::cout << std::string(125, '=') << "\n\n";

    std::cout << "Value Preservation:\n";
    std::cout << "  Original Value:       " << std::fixed << std::setprecision(2) << original_value << "\n";
    std::cout << "  Final Decrypted:      " << std::fixed << std::setprecision(2) << final_val << "\n";
    std::cout << "  Absolute Error:       " << std::fixed << std::setprecision(6) << fabs(final_val - original_value) << "\n";
    std::cout << "  Error Percentage:     " << std::fixed << std::setprecision(6)
              << (100.0 * fabs(final_val - original_value) / original_value) << "%\n";

    bool value_preserved = (fabs(final_val - original_value) < 1.0);
    std::cout << "  Value Preserved:      " << (value_preserved ? "YES" : "NO") << "\n\n";

    std::cout << "Performance:\n";
    std::cout << "  Total Cycles:         " << cycles << "\n";
    std::cout << "  Total Operations:     " << (cycles * 2) << " (add + sub per cycle)\n";
    std::cout << "  Bootstraps:           " << cb.get_bootstrap_count() << "\n";
    std::cout << "  Execution Time:       " << std::fixed << std::setprecision(2) << total_elapsed << "s\n";
    std::cout << "  Throughput:           " << std::fixed << std::setprecision(2)
              << (cycles * 2 / total_elapsed) << " ops/s\n\n";

    ControllerState final_ctrl = cb.get_controller_state();
    MetaControllerState final_meta = cb.get_meta_state();

    std::cout << "Self-Optimizing Controller:\n";
    std::cout << "  PHI Final:            " << std::fixed << std::setprecision(4) << final_ctrl.integrated_phi << "\n";
    std::cout << "  Stability:            " << std::fixed << std::setprecision(4) << final_ctrl.stability << "\n";
    std::cout << "  Learned Threshold:    " << std::fixed << std::setprecision(4) << final_meta.adjusted_threshold << "\n";
    std::cout << "  Final Learn Rate:     " << std::fixed << std::setprecision(4) << final_meta.learning_rate << "\n";
    std::cout << "  Meta Stable at:       " << (meta_stable_at > 0 ? "Cycle " + std::to_string(meta_stable_at) : "NOT REACHED") << "\n";
    std::cout << "  Fractal at:           " << (fractal_at > 0 ? "Cycle " + std::to_string(fractal_at) : "NOT DETECTED") << "\n\n";

    std::cout << "GF-N Integrity:\n";
    std::cout << "  Healthy Layers:       " << cb.get_healthy_layers() << "/" << gf_layers << "\n";
    std::cout << "  Seed Rotations:       " << cb.get_rotation_count() << "\n";
    std::cout << "  Min Cassini:          " << std::fixed << std::setprecision(6) << cb.get_min_cassini() << "\n\n";

    std::cout << "================================================================================\n";
    if (alive && value_preserved && cb.verify_integrity()) {
        std::cout << "  VERDICT: VALUE PRESERVATION CONFIRMED\n";
        std::cout << "  Input: " << original_value << " -> Output: " << final_val
                  << " (error: " << fabs(final_val - original_value) << ")\n";
        std::cout << "  " << cycles << " cycles of add/sub completed. Value intact.\n";
    } else {
        std::cout << "  VERDICT: TEST FAILED\n";
    }
    std::cout << "================================================================================\n\n";

    return (alive && value_preserved) ? 0 : 1;
}
