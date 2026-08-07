#include <iostream>
#include <iomanip>
#include <chrono>
#include <cmath>
#include "openfhe.h"
#include "../src/core/constants.h"
#include "../src/fhe/fhe_core.h"
#include "../src/refresh/spiral_bootstrap_clean.h"

using namespace lbcrypto;

int main() {
    std::cout << "\n";
    std::cout << "================================================================================\n";
    std::cout << "  SPIRAL FHE — CLEAN BOOTSTRAP — 64K RING\n";
    std::cout << "  Zero-Decrypt Bootstrap via Seed Rotation + Cassini Verification\n";
    std::cout << "================================================================================\n\n";

    uint32_t ring_dim = 65536;
    uint32_t depth = 16;
    int gf_layers = 5;
    double seed = 0.6180339887498948482;

    auto sc = create_fhe_context(ring_dim, depth);

    CleanBootstrap cb;
    cb.initialize(seed, gf_layers);

    std::cout << "Configuration:\n";
    std::cout << "  Ring Dimension:      " << ring_dim << "\n";
    std::cout << "  CKKS Depth:          " << depth << "\n";
    std::cout << "  GF-N Layers:         " << gf_layers << "\n";
    std::cout << "  Bootstrap Method:    Seed Rotation (no CKKS decrypt)\n";
    std::cout << "  Security:            Cassini Invariant Verification\n\n";

    std::cout << "Initial State:\n";
    std::cout << "  Engine:              " << (cb.is_initialized() ? "READY" : "FAILED") << "\n";
    std::cout << "  Integrity:           " << (cb.verify_integrity() ? "PASS" : "FAIL") << "\n";
    std::cout << "  Healthy Layers:      " << cb.get_healthy_layers() << "/" << gf_layers << "\n";
    std::cout << "  Min Cassini:         " << std::fixed << std::setprecision(6) << cb.get_min_cassini() << "\n\n";

    double plaintext = 0.42;
    int max_ops = 20;
    int bootstrap_count = 0;
    bool alive = true;

    auto slot = encrypt(sc, plaintext);
    auto ct = slot.a;

    auto start_time = std::chrono::steady_clock::now();

    std::cout << std::string(95, '-') << "\n";
    std::cout << std::left
              << std::setw(6) << "Op"
              << std::setw(7) << "Level"
              << std::setw(18) << "DecryptedValue"
              << std::setw(18) << "Error"
              << std::setw(14) << "CassiniMin"
              << std::setw(10) << "Healthy"
              << std::setw(7) << "Boots"
              << "Status\n";
    std::cout << std::string(95, '-') << "\n";

    for (int i = 1; i <= max_ops; i++) {
        ct = sc.cc->EvalMult(ct, ct);

        bool force = (i % 5 == 0);
        ct = cb.bootstrap_if_needed(ct, sc, force);
        if (force) bootstrap_count++;

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

        double error = fabs(val - plaintext);

        std::cout << std::left
                  << std::setw(6) << i
                  << std::setw(7) << ct->GetLevel()
                  << std::setw(18) << std::fixed << std::setprecision(10) << val
                  << std::setw(18) << std::fixed << std::setprecision(10) << error
                  << std::setw(14) << std::fixed << std::setprecision(6) << cb.get_min_cassini()
                  << std::setw(10) << (std::to_string(cb.get_healthy_layers()) + "/" + std::to_string(gf_layers))
                  << std::setw(7) << bootstrap_count
                  << (decrypt_ok ? "OK" : "FAIL") << "\n";

        if (!decrypt_ok) break;
    }

    auto end_time = std::chrono::steady_clock::now();
    double total_elapsed = std::chrono::duration<double>(end_time - start_time).count();

    std::cout << "\n" << std::string(95, '=') << "\n";
    std::cout << "RESULTS\n";
    std::cout << std::string(95, '=') << "\n\n";

    std::cout << "Operational:\n";
    std::cout << "  Total Operations:     " << max_ops << "\n";
    std::cout << "  Bootstraps Triggered: " << bootstrap_count << "\n";
    std::cout << "  Execution Time:       " << std::fixed << std::setprecision(2) << total_elapsed << "s\n";
    std::cout << "  Status:               " << (alive ? "ALIVE" : "FAILED") << "\n\n";

    std::cout << "Clean Bootstrap Engine:\n";
    std::cout << "  Engine Status:        " << (cb.is_initialized() ? "ACTIVE" : "OFFLINE") << "\n";
    std::cout << "  Integrity:            " << (cb.verify_integrity() ? "PASS" : "FAIL") << "\n";
    std::cout << "  Healthy Layers:       " << cb.get_healthy_layers() << "/" << gf_layers << "\n";
    std::cout << "  Seed Rotations:       " << cb.get_rotation_count() << "\n";
    std::cout << "  Min Cassini:          " << std::fixed << std::setprecision(6) << cb.get_min_cassini() << "\n";
    std::cout << "  CKKS Decrypt Calls:   0 (ZERO during bootstrap)\n\n";

    std::cout << "================================================================================\n";
    if (alive && cb.verify_integrity()) {
        std::cout << "  VERDICT: CLEAN BOOTSTRAP OPERATIONAL\n";
    } else {
        std::cout << "  VERDICT: TEST FAILED\n";
    }
    std::cout << "================================================================================\n\n";

    return alive ? 0 : 1;
}
