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
    std::cout << "  SPIRAL FHE — MULTIPLICATION STRESS TEST — 64K RING\n";
    std::cout << "  EvalMult Squaring + Self-Optimizing Cassini-Driven Bootstrap\n";
    std::cout << "================================================================================\n\n";

    uint32_t ring_dim = 65536;
    uint32_t depth = 32;
    double seed = 0.6180339887498948482;

    auto sc = create_fhe_context(ring_dim, depth);
    CompleteBootstrap<5, 30> cb;
    cb.initialize(seed);

    std::cout << "Configuration:\n";
    std::cout << "  Ring Dimension:      " << ring_dim << "\n";
    std::cout << "  CKKS Depth:          " << depth << "\n";
    std::cout << "  GF-N Layers:         5\n";
    std::cout << "  Operation:           ct = ct * ct (repeated squaring, high noise)\n";
    std::cout << "  Controller:          Cassini-driven recursive fractal\n\n";

    std::cout << "Initial State:\n";
    std::cout << "  Engine:              " << (cb.is_initialized() ? "READY" : "FAILED") << "\n";
    std::cout << "  Integrity:           " << (cb.verify_integrity() ? "PASS" : "FAIL") << "\n";
    std::cout << "  Healthy Layers:      " << cb.get_healthy_layers() << "/5\n";
    std::cout << "  Min Cassini:         " << std::fixed << std::setprecision(6) << cb.get_min_cassini() << "\n\n";

    double plaintext = 0.42;
    int max_ops = 100;
    bool alive = true;
    int fractal_at = -1;
    int meta_stable_at = -1;

    auto slot = encrypt(sc, plaintext);
    auto ct = slot.a;
    auto start_time = std::chrono::steady_clock::now();

    std::cout << std::string(125, '-') << "\n";
    std::cout << std::left
              << std::setw(8) << "Op"
              << std::setw(10) << "Decrypted"
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

    for (int i = 1; i <= max_ops; i++) {
        ct = sc.cc->EvalMult(ct, ct);
        ct = cb.bootstrap_auto(ct, sc);

        if (cb.is_fractal_converged() && fractal_at < 0) fractal_at = i;
        if (cb.parameters_stable() && meta_stable_at < 0) meta_stable_at = i;

        if (i % 5 == 0 || i == 1 || i == max_ops || i == fractal_at || i == meta_stable_at) {
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

            double expected = std::pow(plaintext, std::pow(2, i));
            double error = fabs(val - expected);
            MetaControllerState meta = cb.get_meta_state();
            ControllerState ctrl = cb.get_controller_state();

            std::cout << std::left
                      << std::setw(8) << i
                      << std::setw(10) << std::fixed << std::setprecision(6) << val
                      << std::setw(14) << std::fixed << std::setprecision(8) << error
                      << std::setw(14) << std::fixed << std::setprecision(6) << cb.get_min_cassini()
                      << std::setw(14) << std::fixed << std::setprecision(4) << meta.adjusted_threshold
                      << std::setw(10) << std::fixed << std::setprecision(4) << meta.learning_rate
                      << std::setw(10) << (std::to_string(ctrl.healthy_layers) + "/5")
                      << std::setw(8) << cb.get_bootstrap_count()
                      << std::setw(12) << (meta.parameters_stable ? "YES" : "NO")
                      << std::setw(12) << (cb.is_fractal_converged() ? "DETECTED" : "SEARCHING")
                      << (decrypt_ok ? "OK" : "FAIL") << "\n";

            if (i == fractal_at) std::cout << "  >>> FRACTAL CONVERGENCE at Op " << i << " <<<\n";
            if (i == meta_stable_at) std::cout << "  >>> META PARAMETERS STABLE at Op " << i << " <<<\n";
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
    std::cout << "RESULTS — MULTIPLICATION STRESS TEST\n";
    std::cout << std::string(125, '=') << "\n\n";

    std::cout << "Value Verification:\n";
    std::cout << "  Original:             " << std::fixed << std::setprecision(6) << plaintext << "\n";
    std::cout << "  Final Decrypted:      " << std::fixed << std::setprecision(10) << final_val << "\n";
    std::cout << "  Expected (0.42^(2^" << max_ops << ")): ~0.0\n";
    std::cout << "  Decryption:           " << (final_ok ? "OK" : "FAILED") << "\n\n";

    std::cout << "Performance:\n";
    std::cout << "  Total Operations:     " << max_ops << "\n";
    std::cout << "  Bootstraps:           " << cb.get_bootstrap_count() << "\n";
    std::cout << "  Bootstrap Rate:       " << std::fixed << std::setprecision(1)
              << (100.0 * cb.get_bootstrap_count() / max_ops) << "%\n";
    std::cout << "  Execution Time:       " << std::fixed << std::setprecision(2) << total_elapsed << "s\n";
    std::cout << "  Throughput:           " << std::fixed << std::setprecision(2)
              << (max_ops / total_elapsed) << " ops/s\n\n";

    ControllerState final_ctrl = cb.get_controller_state();
    MetaControllerState final_meta = cb.get_meta_state();

    std::cout << "Cassini-Driven Controller:\n";
    std::cout << "  PHI Final:            " << std::fixed << std::setprecision(4) << final_ctrl.integrated_phi << "\n";
    std::cout << "  Stability:            " << std::fixed << std::setprecision(4) << final_ctrl.stability << "\n";
    std::cout << "  Learned Threshold:    " << std::fixed << std::setprecision(4) << final_meta.adjusted_threshold << "\n";
    std::cout << "  Final Learn Rate:     " << std::fixed << std::setprecision(4) << final_meta.learning_rate << "\n";
    std::cout << "  Meta Stable at:       " << (meta_stable_at > 0 ? "Op " + std::to_string(meta_stable_at) : "NOT REACHED") << "\n";
    std::cout << "  Fractal at:           " << (fractal_at > 0 ? "Op " + std::to_string(fractal_at) : "NOT DETECTED") << "\n\n";

    std::cout << "GF-N Integrity:\n";
    std::cout << "  Healthy Layers:       " << cb.get_healthy_layers() << "/5\n";
    std::cout << "  Seed Rotations:       " << cb.get_rotation_count() << "\n";
    std::cout << "  Min Cassini:          " << std::fixed << std::setprecision(6) << cb.get_min_cassini() << "\n\n";

    std::cout << "================================================================================\n";
    if (alive && cb.verify_integrity()) {
        std::cout << "  VERDICT: STRESS TEST PASSED\n";
        std::cout << "  " << max_ops << " squarings completed. System alive. GF-N integrity maintained.\n";
    } else {
        std::cout << "  VERDICT: TEST FAILED\n";
    }
    std::cout << "================================================================================\n\n";

    return alive ? 0 : 1;
}
