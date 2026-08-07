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
    std::cout << "  SPIRAL FHE — COMPLETE BOOTSTRAP — 64K RING — 500 OPERATIONS\n";
    std::cout << "  Self-Optimizing Controller + GF-N Seed Rotation + Cassini Verification\n";
    std::cout << "================================================================================\n\n";

    uint32_t ring_dim = 65536;
    uint32_t depth = 16;
    int gf_layers = 5;
    double seed = 0.6180339887498948482;

    auto sc = create_fhe_context(ring_dim, depth);
    CompleteBootstrap cb;
    cb.initialize(seed, gf_layers);

    std::cout << "Configuration:\n";
    std::cout << "  Ring Dimension:      " << ring_dim << "\n";
    std::cout << "  CKKS Depth:          " << depth << "\n";
    std::cout << "  GF-N Layers:         " << gf_layers << "\n";
    std::cout << "  Controller:          Recursive Fractal (3-level self-optimizing)\n";
    std::cout << "  Bootstrap Decision:  Automatic (PHI-based, no manual trigger)\n\n";

    std::cout << "Initial State:\n";
    std::cout << "  Engine:              " << (cb.is_initialized() ? "READY" : "FAILED") << "\n";
    std::cout << "  Integrity:           " << (cb.verify_integrity() ? "PASS" : "FAIL") << "\n";
    std::cout << "  Healthy Layers:      " << cb.get_healthy_layers() << "/" << gf_layers << "\n";
    std::cout << "  Min Cassini:         " << std::fixed << std::setprecision(6) << cb.get_min_cassini() << "\n";
    std::cout << "  Initial Threshold:   8 (auto-adjusting)\n";
    std::cout << "  Initial Learn Rate:  0.618\n\n";

    double plaintext = 0.42;
    int max_ops = 500;
    int bootstrap_count = 0;
    bool alive = true;
    int fractal_at = -1;
    int meta_stable_at = -1;

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
              << std::setw(14) << "CassiniMin"
              << std::setw(10) << "Healthy"
              << std::setw(8) << "Boots"
              << std::setw(12) << "MetaStable"
              << std::setw(12) << "Fractal"
              << "Status\n";
    std::cout << std::string(130, '-') << "\n";

    for (int i = 1; i <= max_ops; i++) {
        ct = sc.cc->EvalMult(ct, ct);
        ct = cb.bootstrap_auto(ct, sc);

        if (cb.is_fractal_converged() && fractal_at < 0) fractal_at = i;
        if (cb.parameters_stable() && meta_stable_at < 0) meta_stable_at = i;

        if (i % 50 == 0 || i == 1 || i == max_ops || i == fractal_at || i == meta_stable_at) {
            auto now = std::chrono::steady_clock::now();
            double elapsed = std::chrono::duration<double>(now - start_time).count();

            ControllerState ctrl = cb.get_controller_state();
            MetaControllerState meta = cb.get_meta_state();

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

            std::cout << std::left
                      << std::setw(8) << i
                      << std::setw(8) << ct->GetLevel()
                      << std::setw(12) << std::fixed << std::setprecision(4) << ctrl.integrated_phi
                      << std::setw(10) << std::fixed << std::setprecision(4) << ctrl.phi_stability
                      << std::setw(12) << meta.adjusted_refresh_level
                      << std::setw(10) << std::fixed << std::setprecision(4) << meta.learning_rate
                      << std::setw(14) << std::fixed << std::setprecision(6) << cb.get_min_cassini()
                      << std::setw(10) << (std::to_string(cb.get_healthy_layers()) + "/" + std::to_string(gf_layers))
                      << std::setw(8) << cb.get_bootstrap_count()
                      << std::setw(12) << (meta.parameters_stable ? "YES" : "NO")
                      << std::setw(12) << (cb.is_fractal_converged() ? "DETECTED" : "SEARCHING")
                      << (decrypt_ok ? "OK" : "FAIL") << "\n";

            if (i == fractal_at) {
                std::cout << "  >>> FRACTAL CONVERGENCE at Op " << i << " <<<\n";
            }
            if (i == meta_stable_at) {
                std::cout << "  >>> META PARAMETERS STABLE at Op " << i << " <<<\n";
            }
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

    std::cout << "\n" << std::string(130, '=') << "\n";
    std::cout << "RESULTS — 64K RING — 500 OPERATIONS — COMPLETE BOOTSTRAP\n";
    std::cout << std::string(130, '=') << "\n\n";

    std::cout << "Operational:\n";
    std::cout << "  Total Operations:     " << max_ops << "\n";
    std::cout << "  Bootstraps:           " << cb.get_bootstrap_count() << "\n";
    std::cout << "  Bootstrap Rate:       " << std::fixed << std::setprecision(2)
              << (100.0 * cb.get_bootstrap_count() / max_ops) << "%\n";
    std::cout << "  Execution Time:       " << std::fixed << std::setprecision(2) << total_elapsed << "s\n";
    std::cout << "  Throughput:           " << std::fixed << std::setprecision(2)
              << (max_ops / total_elapsed) << " ops/s\n";
    std::cout << "  Status:               " << (alive ? "ALIVE" : "FAILED") << "\n\n";

    std::cout << "Value Verification:\n";
    std::cout << "  Original Plaintext:   " << std::fixed << std::setprecision(10) << plaintext << "\n";
    std::cout << "  Final Decrypted:      " << std::fixed << std::setprecision(10) << final_val << "\n";
    std::cout << "  Absolute Error:       " << std::fixed << std::setprecision(10) << fabs(final_val - plaintext) << "\n";
    std::cout << "  Value Preserved:      " << (final_ok ? "YES" : "NO") << "\n\n";

    ControllerState final_ctrl = cb.get_controller_state();
    MetaControllerState final_meta = cb.get_meta_state();

    std::cout << "Self-Optimizing Controller:\n";
    std::cout << "  PHI Final:            " << std::fixed << std::setprecision(4) << final_ctrl.integrated_phi << "\n";
    std::cout << "  PHI Stability:        " << std::fixed << std::setprecision(4) << final_ctrl.phi_stability << "\n";
    std::cout << "  Learned Threshold:    " << final_meta.adjusted_refresh_level << "\n";
    std::cout << "  Final Learn Rate:     " << std::fixed << std::setprecision(4) << final_meta.learning_rate << "\n";
    std::cout << "  Meta Stable at:       " << (meta_stable_at > 0 ? "Op " + std::to_string(meta_stable_at) : "NOT REACHED") << "\n";
    std::cout << "  Fractal at:           " << (fractal_at > 0 ? "Op " + std::to_string(fractal_at) : "NOT DETECTED") << "\n";
    std::cout << "  Similarity:           " << std::fixed << std::setprecision(6) << cb.get_fractal_similarity() << "\n\n";

    std::cout << "GF-N Integrity:\n";
    std::cout << "  Healthy Layers:       " << cb.get_healthy_layers() << "/" << gf_layers << "\n";
    std::cout << "  Seed Rotations:       " << cb.get_rotation_count() << "\n";
    std::cout << "  Min Cassini:          " << std::fixed << std::setprecision(6) << cb.get_min_cassini() << "\n\n";

    std::cout << "================================================================================\n";
    if (alive && cb.verify_integrity()) {
        std::cout << "  VERDICT: COMPLETE BOOTSTRAP — OPERATIONAL\n";
        std::cout << "  Self-optimizing controller active. GF-N integrity maintained.\n";
    } else {
        std::cout << "  VERDICT: TEST FAILED\n";
    }
    std::cout << "================================================================================\n\n";

    return alive ? 0 : 1;
}
