#include <iostream>
#include <iomanip>
#include <chrono>
#include <cmath>
#include <string>
#include "openfhe.h"
#include "../src/core/constants.h"
#include "../src/fhe/fhe_core.h"
#include "../src/refresh/spiral_bootstrap.h"

using namespace lbcrypto;

struct TestConfig {
    uint32_t ring_dim;
    uint32_t depth;
    int max_ops;
    double plaintext;
    std::string operation;
    std::string description;
};

template<int N>
void run_test(const TestConfig& config) {
    std::cout << "\n" << std::string(90, '=') << "\n";
    std::cout << "  " << config.description << "\n";
    std::cout << std::string(90, '=') << "\n\n";

    auto sc = create_fhe_context(config.ring_dim, config.depth);

    std::cout << "Config: ring=" << config.ring_dim << " depth=" << config.depth 
              << " ops=" << config.max_ops << " plaintext=" << std::fixed << std::setprecision(4) << config.plaintext << "\n\n";

    CompleteBootstrap<N, 30> cb;
    cb.initialize(0.6180339887498948482);

    std::cout << "Init: healthy=" << cb.get_healthy_layers() << "/" << N 
              << " cassini=" << std::fixed << std::setprecision(4) << cb.get_min_cassini() << "\n\n";

    auto ct = encrypt(sc, config.plaintext);
    auto ct_half = encrypt(sc, 0.5);
    auto ct_two = encrypt(sc, 2.0);

    bool alive = true;
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
        } else if (config.operation == "chained") {
            ct = sc.cc->EvalAdd(ct, ct_half);
            ct = cb.bootstrap_auto(ct, sc);
            ct = sc.cc->EvalMult(ct, ct_two);
            ct = cb.bootstrap_auto(ct, sc);
            ct = sc.cc->EvalSub(ct, ct_half);
        }

        ct = cb.bootstrap_auto(ct, sc);

        if (i % (config.max_ops / 5) == 0 || i == config.max_ops) {
            double val = 0.0;
            try {
                val = decrypt(sc, ct);
            } catch (const std::exception& e) {
                alive = false;
            }

            MetaControllerState meta = cb.get_meta_state();
            double rate = (100.0 * cb.get_bootstrap_count() / i);

            std::cout << "  Op " << std::setw(4) << i 
                      << " | val=" << std::fixed << std::setprecision(6) << std::setw(10) << val
                      << " | cass=" << std::fixed << std::setprecision(3) << cb.get_min_cassini()
                      << " | thresh=" << std::fixed << std::setprecision(3) << meta.adjusted_threshold
                      << " | minLvl=" << meta.learned_min_level
                      << " | LR=" << std::fixed << std::setprecision(2) << meta.learning_rate
                      << " | HL=" << cb.get_healthy_layers() << "/" << N
                      << " | boots=" << cb.get_bootstrap_count()
                      << " | " << std::fixed << std::setprecision(0) << rate << "%"
                      << " | " << (alive ? "OK" : "FAIL") << "\n";
            if (!alive) break;
        }
    }

    auto end_time = std::chrono::steady_clock::now();
    double elapsed = std::chrono::duration<double>(end_time - start_time).count();

    double final_val = 0.0;
    try { final_val = decrypt(sc, ct); } catch (...) { alive = false; }

    std::cout << "\n  Result: " << config.max_ops << " ops, " << cb.get_bootstrap_count() 
              << " boots, " << std::fixed << std::setprecision(1) << elapsed << "s"
              << ", final=" << std::fixed << std::setprecision(6) << final_val
              << ", " << (alive ? "ALIVE" : "FAILED") << "\n";
}

int main() {
    std::cout << "\n";
    std::cout << "================================================================================\n";
    std::cout << "  SpiralFHE — Test Suite\n";
    std::cout << "  Self-Optimizing Bootstrap with Cassini-Verified Seed Rotation\n";
    std::cout << "================================================================================\n";

    std::vector<TestConfig> tests = {
        {65536, 16, 20, 0.42, "add_const", "Add Constants (0.42 + 0.5 per op)"},
        {65536, 16, 20, 0.42, "mult_const", "Multiply Constants (0.42 * 0.5 per op)"},
        {65536, 16, 10, 0.42, "rotate", "Cyclic Rotations"},
        {65536, 16, 10, 0.42, "sum", "Sum All Slots"},
        {65536, 16, 20, 0.42, "add_sub_cycle", "Add/Sub Cycles (value preservation)"},
        {65536, 32, 15, 0.42, "chained", "Chained Ops (Add -> Mult -> Sub)"},
        {65536, 16, 15, 0.42, "square", "Squaring (stress test)"},
        {65536, 32, 30, 24325.0, "add_sub_cycle", "Value Preservation (24325.00)"},
    };

    for (auto& t : tests) {
        run_test<5>(t);
    }

    std::cout << "\n" << std::string(90, '=') << "\n";
    std::cout << "  Test Suite Complete\n";
    std::cout << std::string(90, '=') << "\n\n";

    return 0;
}
