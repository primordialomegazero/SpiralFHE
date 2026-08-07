#include <iostream>
#include <iomanip>
#include <chrono>
#include <cmath>
#include <string>
#include "openfhe.h"
#include "../src/core/constants.h"
#include "../src/fhe/fhe_core.h"
#include "../src/refresh/spiral_bootstrap_complete.h"

using namespace lbcrypto;

struct TestConfig {
    uint32_t ring_dim;
    uint32_t depth;
    int gf_layers;
    int history_size;
    int max_ops;
    double plaintext;
    std::string operation;
    std::string description;
};

template<int N>
void run_test_with_layers(const TestConfig& config) {
    std::cout << "\n" << std::string(120, '=') << "\n";
    std::cout << "  " << config.description << "\n";
    std::cout << std::string(120, '=') << "\n\n";

    auto sc = create_fhe_context(config.ring_dim, config.depth);

    std::cout << "Configuration:\n";
    std::cout << "  Ring Dimension:      " << config.ring_dim << "\n";
    std::cout << "  CKKS Depth:          " << config.depth << "\n";
    std::cout << "  GF-N Layers:         " << N << "\n";
    std::cout << "  History Size:        " << config.history_size << "\n";
    std::cout << "  Max Operations:      " << config.max_ops << "\n";
    std::cout << "  Operation:           " << config.operation << "\n";
    std::cout << "  Plaintext:           " << std::fixed << std::setprecision(6) << config.plaintext << "\n\n";

    CompleteBootstrap<N, 30> cb;
    cb.initialize(0.6180339887498948482);

    std::cout << "Initial State:\n";
    std::cout << "  Engine:              " << (cb.is_initialized() ? "READY" : "FAILED") << "\n";
    std::cout << "  Integrity:           " << (cb.verify_integrity() ? "PASS" : "FAIL") << "\n";
    std::cout << "  Healthy Layers:      " << cb.get_healthy_layers() << "/" << N << "\n";
    std::cout << "  Min Cassini:         " << std::fixed << std::setprecision(6) << cb.get_min_cassini() << "\n\n";

    auto ct = encrypt(sc, config.plaintext);
    auto ct_half = encrypt(sc, 0.5);
    auto ct_two = encrypt(sc, 2.0);

    bool alive = true;
    int meta_stable_at = -1;
    int fractal_at = -1;
    auto start_time = std::chrono::steady_clock::now();

    for (int i = 1; i <= config.max_ops; i++) {
        if (config.operation == "square") {
            ct = sc.cc->EvalMult(ct, ct);
        } else if (config.operation == "add_const") {
            ct = sc.cc->EvalAdd(ct, ct_half);
        } else if (config.operation == "mult_const") {
            ct = sc.cc->EvalMult(ct, ct_half);
        } else if (config.operation == "add_sub_cycle") {
            ct = sc.cc->EvalAdd(ct, ct_half);
            ct = cb.bootstrap_auto(ct, sc);
            ct = sc.cc->EvalSub(ct, ct_half);
        } else if (config.operation == "rotate") {
            ct = sc.cc->EvalRotate(ct, 1);
        } else if (config.operation == "sum") {
            ct = sc.cc->EvalSum(ct, config.ring_dim / 2);
        }

        ct = cb.bootstrap_auto(ct, sc);

        if (cb.is_fractal_converged() && fractal_at < 0) fractal_at = i;
        if (cb.parameters_stable() && meta_stable_at < 0) meta_stable_at = i;

        if (i % (config.max_ops / 5) == 0 || i == config.max_ops || i == fractal_at || i == meta_stable_at) {
            double val = 0.0;
            try {
                val = decrypt(sc, ct);
            } catch (const std::exception& e) {
                alive = false;
            }

            MetaControllerState meta = cb.get_meta_state();
            ControllerState ctrl = cb.get_controller_state();
            double rate = (100.0 * cb.get_bootstrap_count() / i);

            std::cout << "  Op " << std::setw(3) << i 
                      << " | Val: " << std::fixed << std::setprecision(4) << val
                      << " | Cassini: " << std::fixed << std::setprecision(3) << cb.get_min_cassini()
                      << " | Thresh: " << std::fixed << std::setprecision(3) << meta.adjusted_threshold
                      << " | MinLvl: " << meta.learned_min_level
                      << " | LR: " << std::fixed << std::setprecision(2) << meta.learning_rate
                      << " | HL: " << ctrl.healthy_layers << "/" << N
                      << " | Boots: " << cb.get_bootstrap_count()
                      << " | " << std::fixed << std::setprecision(0) << rate << "%"
                      << " | " << (alive ? "OK" : "FAIL") << "\n";

            if (i == fractal_at) std::cout << "  >>> FRACTAL CONVERGENCE at Op " << i << " <<<\n";
            if (i == meta_stable_at) std::cout << "  >>> META PARAMETERS STABLE at Op " << i << " <<<\n";
            if (!alive) break;
        }
    }

    auto end_time = std::chrono::steady_clock::now();
    double total_elapsed = std::chrono::duration<double>(end_time - start_time).count();

    double final_val = 0.0;
    try {
        final_val = decrypt(sc, ct);
    } catch (const std::exception& e) {
        alive = false;
    }

    std::cout << "\n  Done: " << config.max_ops << " ops, " << cb.get_bootstrap_count() << " boots, "
              << std::fixed << std::setprecision(1) << total_elapsed << "s, "
              << "final=" << std::fixed << std::setprecision(4) << final_val << ", "
              << (alive ? "ALIVE" : "FAILED") << "\n\n";
}

void run_test(const TestConfig& config) {
    switch(config.gf_layers) {
        case 3: run_test_with_layers<3>(config); break;
        case 5: run_test_with_layers<5>(config); break;
        default: run_test_with_layers<5>(config); break;
    }
}

int main() {
    std::cout << "\n";
    std::cout << "================================================================================\n";
    std::cout << "  SPIRAL FHE — 64K RING DEMO\n";
    std::cout << "  5 Tests: Add, Mult, Rotate, Sum, Add/Sub Cycle\n";
    std::cout << "================================================================================\n";

    std::vector<TestConfig> tests = {
        {65536, 16, 5, 20, 15, 0.42, "add_const", "TEST 1/5: 64K Ring, 15 Add-Constants"},
        {65536, 16, 5, 20, 15, 0.42, "mult_const", "TEST 2/5: 64K Ring, 15 Mult-Constants"},
        {65536, 16, 5, 20, 10, 0.42, "rotate", "TEST 3/5: 64K Ring, 10 Rotations"},
        {65536, 16, 5, 20, 10, 0.42, "sum", "TEST 4/5: 64K Ring, 10 Sum-All-Slots"},
        {65536, 16, 5, 20, 15, 0.42, "add_sub_cycle", "TEST 5/5: 64K Ring, 15 Add/Sub Cycles"},
    };

    int executed = 0;
    auto suite_start = std::chrono::steady_clock::now();

    for (auto& test : tests) {
        run_test(test);
        executed++;
    }

    auto suite_end = std::chrono::steady_clock::now();
    double suite_time = std::chrono::duration<double>(suite_end - suite_start).count();

    std::cout << "\n" << std::string(120, '=') << "\n";
    std::cout << "  ALL " << executed << " TESTS COMPLETE — Total time: " 
              << std::fixed << std::setprecision(1) << suite_time << "s\n";
    std::cout << std::string(120, '=') << "\n\n";

    return 0;
}
