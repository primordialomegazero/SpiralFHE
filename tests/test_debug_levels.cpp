#include <iostream>
#include "openfhe.h"
#include "../src/core/constants.h"
#include "../src/fhe/fhe_core.h"
#include "../src/refresh/spiral_bootstrap.h"

using namespace lbcrypto;

int main() {
    uint32_t ring_dim = 8192;
    uint32_t depth = 16;
    int gf_layers = 3;
    double master_seed = 0.6180339887498948482;

    std::cout << "\n=== LEVEL DEBUG ===\n\n";

    auto sc = create_fhe_context(ring_dim, depth);
    std::cout << "Context created. Max depth: " << depth << "\n";

    SpiralBootstrap bootstrap;
    bootstrap.init(master_seed, gf_layers);
    bootstrap.set_max_level(depth);

    double plaintext = 0.42;
    auto slot = encrypt(sc, plaintext);
    auto ct = slot.a;

    std::cout << "\nInitial level: " << ct->GetLevel() << "\n";

    // Multiply once
    ct = sc.cc->EvalMult(ct, ct);
    std::cout << "After 1x EvalMult: level=" << ct->GetLevel() << "\n";

    // Multiply again
    ct = sc.cc->EvalMult(ct, ct);
    std::cout << "After 2x EvalMult: level=" << ct->GetLevel() << "\n";

    // Bootstrap
    std::cout << "\nCalling bootstrap_zero...\n";
    ct = bootstrap.bootstrap_zero(ct, sc);
    std::cout << "After bootstrap_zero: level=" << ct->GetLevel() << "\n";

    // Multiply after bootstrap
    ct = sc.cc->EvalMult(ct, ct);
    std::cout << "After 1x EvalMult (post-bootstrap): level=" << ct->GetLevel() << "\n";

    // Check max level
    std::cout << "\nbootstrap.max_level = " << bootstrap.max_level << "\n";
    std::cout << "ct->GetNoiseScaleDeg() = " << ct->GetNoiseScaleDeg() << "\n";

    return 0;
}
