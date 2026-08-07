#include <iostream>
#include <iomanip>
#include <vector>
#include <chrono>
#include <cmath>
#include "openfhe.h"
#include "../../src/core/constants.h"

using namespace lbcrypto;

int main() {
    std::cout << "\n";
    std::cout << "╔══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  ENCRYPTED ML INFERENCE IN FHE                                ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════════╝\n\n";
    
    // ═══════════════════════════════════════════════════════════
    // SETUP FHE — Higher depth for more operations
    // ═══════════════════════════════════════════════════════════
    CCParams<CryptoContextCKKSRNS> params;
    params.SetMultiplicativeDepth(10);
    params.SetScalingModSize(50);
    params.SetBatchSize(256);
    params.SetRingDim(16384);  // Higher ring dim for stability
    params.SetSecretKeyDist(UNIFORM_TERNARY);
    params.SetSecurityLevel(HEStd_NotSet);
    
    auto cc = GenCryptoContext(params);
    cc->Enable(PKE); cc->Enable(KEYSWITCH); cc->Enable(LEVELEDSHE);
    auto kp = cc->KeyGen(); cc->EvalMultKeyGen(kp.secretKey);
    std::cout << "  [OK] RingDim=16384, Depth=10\n\n";
    
    // ═══════════════════════════════════════════════════════════
    // SIMPLE ML: 1 neuron, 2 inputs
    // ═══════════════════════════════════════════════════════════
    std::cout << "--- Simple Neuron: 2 inputs → 1 output ---\n";
    std::cout << "  Formula: y = ReLU(w1*x1 + w2*x2 + b)\n\n";
    
    double x1 = 0.5, x2 = 0.3;
    double w1 = 0.8, w2 = -0.4, b = 0.1;
    
    double expected = std::max(0.0, w1*x1 + w2*x2 + b);
    std::cout << "  Expected: y = max(0, 0.8*0.5 + (-0.4)*0.3 + 0.1)\n";
    std::cout << "  Expected: y = " << std::fixed << std::setprecision(4) << expected << "\n\n";
    
    // ═══════════════════════════════════════════════════════════
    // ENCRYPTED INFERENCE
    // ═══════════════════════════════════════════════════════════
    std::cout << "--- Encrypted Computation ---\n";
    
    auto start = std::chrono::steady_clock::now();
    
    // Encrypt inputs (as packed vectors)
    std::vector<double> x1_vec(256, x1);
    std::vector<double> x2_vec(256, x2);
    std::vector<double> w1_vec(256, w1);
    std::vector<double> w2_vec(256, w2);
    std::vector<double> b_vec(256, b);
    
    auto enc_x1 = cc->Encrypt(kp.publicKey, cc->MakeCKKSPackedPlaintext(x1_vec));
    auto enc_x2 = cc->Encrypt(kp.publicKey, cc->MakeCKKSPackedPlaintext(x2_vec));
    auto enc_w1 = cc->Encrypt(kp.publicKey, cc->MakeCKKSPackedPlaintext(w1_vec));
    auto enc_w2 = cc->Encrypt(kp.publicKey, cc->MakeCKKSPackedPlaintext(w2_vec));
    auto enc_b = cc->Encrypt(kp.publicKey, cc->MakeCKKSPackedPlaintext(b_vec));
    
    std::cout << "  [OK] All values encrypted\n";
    
    // w1*x1
    auto term1 = cc->EvalMult(enc_x1, enc_w1);
    std::cout << "  [OK] w1*x1 computed\n";
    
    // w2*x2
    auto term2 = cc->EvalMult(enc_x2, enc_w2);
    std::cout << "  [OK] w2*x2 computed\n";
    
    // w1*x1 + w2*x2
    auto sum12 = cc->EvalAdd(term1, term2);
    std::cout << "  [OK] w1*x1 + w2*x2 computed\n";
    
    // + bias
    auto total = cc->EvalAdd(sum12, enc_b);
    std::cout << "  [OK] + bias computed\n";
    
    // Decrypt result
    Plaintext pt;
    cc->Decrypt(kp.secretKey, total, &pt);
    auto vals = pt->GetCKKSPackedValue();
    double result = vals[0].real();
    
    auto end = std::chrono::steady_clock::now();
    double sec = std::chrono::duration<double>(end - start).count();
    
    // ReLU
    double fhe_output = std::max(0.0, result);
    
    std::cout << "\n  FHE Result: " << std::fixed << std::setprecision(4) << result << "\n";
    std::cout << "  After ReLU: " << fhe_output << "\n";
    std::cout << "  Expected:   " << expected << "\n";
    
    bool match = (std::abs(fhe_output - expected) < 0.01);
    std::cout << "  Match: " << (match ? "YES" : "NO") << "\n";
    std::cout << "  Time: " << sec << " sec\n";
    std::cout << "  Operations: 2 multiplies + 2 adds = 4 FHE ops\n";
    
    if (match) {
        std::cout << "\n╔══════════════════════════════════════════════════════════════╗\n";
        std::cout << "║  ENCRYPTED ML INFERENCE — WORKING!                            ║\n";
        std::cout << "╚══════════════════════════════════════════════════════════════╝\n";
    }
    
    std::cout << "\n";
    return match ? 0 : 1;
}
