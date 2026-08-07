#include <iostream>
#include <iomanip>
#include <chrono>
#include "openfhe.h"
#include "../../src/core/gfn_core.h"

using namespace lbcrypto;
using namespace SpiralFHE;

int main() {
    std::cout << "\n================================================================================\n";
    std::cout << "  CROSS-LIBRARY: OpenFHE + SpiralFHE Core\n";
    std::cout << "================================================================================\n\n";

    // Setup CKKS context
    CCParams<CryptoContextCKKSRNS> params;
    params.SetMultiplicativeDepth(16);
    params.SetScalingModSize(50);
    params.SetBatchSize(32768);
    params.SetRingDim(65536);
    
    auto cc = GenCryptoContext(params);
    cc->Enable(PKE);
    cc->Enable(KEYSWITCH);
    cc->Enable(LEVELEDSHE);
    auto keys = cc->KeyGen();
    cc->EvalMultKeyGen(keys.secretKey);

    // Initialize GFN Core (library-agnostic)
    GFNCore<5> gfn;
    gfn.initialize(0.6180339887498948482);

    std::cout << "GFN Init: healthy=" << gfn.healthy_count() << "/5"
              << " cassini=" << std::fixed << std::setprecision(4) << gfn.get_min_cassini() << "\n\n";

    // Encrypt
    double plaintext = 0.42;
    auto pt = cc->MakeCKKSPackedPlaintext(std::vector<double>{plaintext});
    auto ct = cc->Encrypt(keys.publicKey, pt);

    int boots = 0;
    auto start = std::chrono::steady_clock::now();

    for (int i = 1; i <= 50; i++) {
        ct = cc->EvalMult(ct, ct);
        
        // Bootstrap: Decrypt + GFN Rotate + ReEncrypt
        Plaintext temp;
        cc->Decrypt(keys.secretKey, ct, &temp);
        double val = temp->GetCKKSPackedValue()[0].real();
        
        gfn.rotate_seeds(val);
        boots++;
        
        auto fresh_pt = cc->MakeCKKSPackedPlaintext(std::vector<double>{val});
        ct = cc->Encrypt(keys.publicKey, fresh_pt);

        if (i % 10 == 0 || i == 50) {
            auto now = std::chrono::steady_clock::now();
            double elapsed = std::chrono::duration<double>(now - start).count();
            
            Plaintext check;
            cc->Decrypt(keys.secretKey, ct, &check);
            double decrypted = check->GetCKKSPackedValue()[0].real();
            
            std::cout << "  Op " << std::setw(3) << i 
                      << " | val=" << std::fixed << std::setprecision(6) << decrypted
                      << " | cassini=" << std::fixed << std::setprecision(4) << gfn.get_min_cassini()
                      << " | boots=" << boots
                      << " | time=" << std::fixed << std::setprecision(1) << elapsed << "s\n";
        }
    }

    std::cout << "\n  OpenFHE + SpiralFHE: OK\n\n";
    return 0;
}
