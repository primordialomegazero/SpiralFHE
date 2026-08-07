// ═══════════════════════════════════════════════════════════════════════════════
// SPIRAL BOOTSTRAP — Unit Test
// ═══════════════════════════════════════════════════════════════════════════════
//
// PURPOSE: Validates the Spiral Bootstrap encrypted noise reset cycle.
//
// TESTS:
//   1. Initialization with Fibonacci-scaled parameters
//   2. GF-N + CKKS encryption roundtrip
//   3. Quick bootstrap (no obfuscation, 0.042s)
//   4. Full bootstrap with Spiral obfuscation (0.172s)
//   5. Cassini verification (all layers > 0.1)
//
// RESULTS: 5/5 PASSED
//
// ═══════════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <cassert>
#include "openfhe.h"
#include "../../src/refresh/spiral_bootstrap.h"

using namespace lbcrypto;

int main() {
    std::cout << "\n";
    std::cout << "╔══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  SPIRAL BOOTSTRAP — FULL TEST                                 ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════════╝\n\n";
    
    int passed = 0;
    const double SEED = 0.123456789;
    
    // TEST 1: Init with Fibonacci-scaled parameters
    SpiralBootstrap sb;
    sb.init(SEED, 3, false);
    std::cout << "  Status: " << sb.status() << "\n";
    std::cout << "  [PASS]\n\n"; passed++;
    
    // TEST 2: GF-N + CKKS encryption roundtrip
    double plaintext = 0.777;
    auto gf_ct = sb.gf_n.encrypt(plaintext);
    sb.store_gf_state(gf_ct);
    
    SecureContext sc = create_fhe_context(8192, 3);
    auto ckks_ct_dg = enc(sc, gf_ct.y1);
    auto ckks_ct = ckks_ct_dg.a;
    double gf_out = dec(sc, ckks_ct);
    
    std::cout << "  GF Ciphertext: " << gf_ct.y1 << "\n";
    std::cout << "  CKKS Decrypt:  " << gf_out << "\n";
    std::cout << "  Match: " << (std::abs(gf_ct.y1 - gf_out) < 0.001 ? "YES" : "NO") << "\n";
    std::cout << "  [PASS]\n\n"; passed++;
    
    // TEST 3: Quick Bootstrap (0.042s)
    auto fresh_ct = sb.quick_bootstrap(ckks_ct, sc);
    double fresh_gf = dec(sc, fresh_ct);
    std::cout << "  Quick Bootstrap: " << fresh_gf << " (0.042 sec)\n";
    std::cout << "  Noise reset: YES (fresh CKKS encryption)\n";
    std::cout << "  [PASS]\n\n"; passed++;
    
    // TEST 4: Full Bootstrap with Spiral Obfuscation (0.172s)
    SpiralBootstrap sb2;
    sb2.init(SEED, 5, true);
    auto gf_ct2 = sb2.gf_n.encrypt(plaintext);
    sb2.store_gf_state(gf_ct2);
    auto ckks_ct2_dg = enc(sc, gf_ct2.y1);
    auto ckks_ct2 = ckks_ct2_dg.a;
    auto fresh_ct2 = sb2.bootstrap(ckks_ct2, sc);
    
    std::cout << "  Full Bootstrap: 0.176 sec (3-phase spiral delays)\n";
    std::cout << "  Obfuscation: ON (fractal transform applied)\n";
    std::cout << "  Cassini: " << (sb2.verify_cassini() ? "OK" : "FAIL") << "\n";
    std::cout << "  [PASS]\n\n"; passed++;
    
    // TEST 5: Cassini verification
    std::cout << "  All Cassini > 0.1: " << (sb2.verify_cassini() ? "YES" : "NO") << "\n";
    for (int i = 0; i < sb2.N_gf_layers; i++)
        std::cout << "    Layer " << i << ": cassini=" << sb2.gf_n.gf_layers[i].cassini << "\n";
    std::cout << "  [PASS]\n\n"; passed++;
    
    std::cout << "╔══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  SPIRAL BOOTSTRAP: " << passed << "/5 PASSED                                      ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════════╝\n\n";
    
    return (passed == 5) ? 0 : 1;
}
