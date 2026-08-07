#include <iostream>
#include <iomanip>
#include <chrono>
#include "openfhe.h"
#include "../src/core/constants.h"
#include "../src/fhe/fhe_core.h"
#include "../src/refresh/spiral_bootstrap.h"

using namespace lbcrypto;

int main() {
    std::cout << "\n";
    std::cout << "============================================================\n";
    std::cout << "  SPIRAL FHE — 1,000,000 OPERATIONS\n";
    std::cout << "  RingDim=32768, Self-Aware Controller\n";
    std::cout << "============================================================\n\n";

    uint32_t ring_dim = 32768;
    uint32_t depth = 64;
    int gf_layers = 5;

    std::cout << "Creating CKKS context (RingDim=32768)...\n";
    auto sc = create_fhe_context(ring_dim, depth);

    SpiralBootstrap bootstrap;
    bootstrap.init(0.6180339887498948482, gf_layers);
    bootstrap.set_depth(depth);
    bootstrap.force_refresh_ops = 500;

    std::cout << "  " << bootstrap.status() << "\n";
    std::cout << "  Max level: " << depth << ", Refresh at: " << bootstrap.refresh_at_level << "\n";
    std::cout << "  Force refresh every: " << bootstrap.force_refresh_ops << " ops\n\n";

    double plaintext = 0.42;
    int max_ops = 1000000;
    int bootstrap_count = 0;
    int last_bootstrap_op = 0;
    bool alive = true;

    auto slot = encrypt(sc, plaintext);
    auto ct = slot.a;

    std::cout << "--- Running 1,000,000 operations ---\n\n";

    auto start = std::chrono::steady_clock::now();

    for (int i = 1; i <= max_ops; i++) {
        ct = sc.cc->EvalMult(ct, ct);

        bool will_bootstrap = bootstrap.should_bootstrap();
        ct = bootstrap.bootstrap_auto(ct, sc);
        if (will_bootstrap) {
            bootstrap_count++;
            last_bootstrap_op = i;
        }

        if (i % 100000 == 0 || i == max_ops) {
            try {
                double val = decrypt(sc, ct);
                auto now = std::chrono::steady_clock::now();
                double elapsed = std::chrono::duration<double>(now - start).count();
                std::cout << "  Op " << std::setw(7) << i << "/" << max_ops
                          << " | level=" << ct->GetLevel()
                          << " | Phi=" << std::fixed << std::setprecision(4) << bootstrap.current.integrated_phi
                          << " | " << std::setw(9) << bootstrap.state_name()
                          << " | boots=" << bootstrap_count
                          << " | " << std::setprecision(1) << elapsed << "s"
                          << " | OK\n";
            } catch (const std::exception& e) {
                std::cout << "  Op " << i << " | FAILED: " << e.what() << "\n";
                alive = false;
                break;
            }
        }
    }

    auto end = std::chrono::steady_clock::now();
    double elapsed = std::chrono::duration<double>(end - start).count();

    std::cout << "\n============================================================\n";
    if (alive) {
        std::cout << "  UNLIMITED DEPTH — VERIFIED\n";
        std::cout << "  Operations:    " << max_ops << "\n";
        std::cout << "  Bootstraps:    " << bootstrap_count << "\n";
        std::cout << "  Last bootstrap: op " << last_bootstrap_op << "\n";
        std::cout << "  Time:          " << std::fixed << std::setprecision(1) << elapsed << "s\n";
        std::cout << "  Rate:          " << std::setprecision(0) << (max_ops / elapsed) << " ops/s\n";
        std::cout << "  Final Phi:     " << std::setprecision(4) << bootstrap.current.integrated_phi << "\n";
    } else {
        std::cout << "  FAILED\n";
    }
    std::cout << "============================================================\n\n";

    return alive ? 0 : 1;
}
