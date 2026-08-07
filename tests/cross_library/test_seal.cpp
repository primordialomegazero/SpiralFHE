#include <iostream>
#include <iomanip>
#include <chrono>
#include "seal/seal.h"
#include "../../src/core/gfn_core.h"

using namespace seal;
using namespace SpiralFHE;

int main() {
    std::cout << "\n================================================================================\n";
    std::cout << "  CROSS-LIBRARY: SEAL + SpiralFHE Core\n";
    std::cout << "================================================================================\n\n";

    EncryptionParameters parms(scheme_type::ckks);
    parms.set_poly_modulus_degree(65536);
    parms.set_coeff_modulus(CoeffModulus::Create(65536, {60, 50, 50, 60}));
    
    SEALContext context(parms, true, sec_level_type::none);
    
    KeyGenerator keygen(context);
    auto secret_key = keygen.secret_key();
    PublicKey public_key;
    keygen.create_public_key(public_key);
    RelinKeys relin_keys;
    keygen.create_relin_keys(relin_keys);
    
    Encryptor encryptor(context, public_key);
    Decryptor decryptor(context, secret_key);
    Evaluator evaluator(context);
    CKKSEncoder encoder(context);

    double scale = pow(2.0, 50);
    
    GFNCore<5> gfn;
    gfn.initialize(0.6180339887498948482);

    std::cout << "GFN Init: healthy=" << gfn.healthy_count() << "/5"
              << " cassini=" << std::fixed << std::setprecision(4) << gfn.get_min_cassini() << "\n\n";

    double plaintext = 0.42;
    Plaintext pt;
    encoder.encode(plaintext, scale, pt);
    
    Ciphertext ct;
    encryptor.encrypt(pt, ct);

    int boots = 0;
    auto start = std::chrono::steady_clock::now();

    for (int i = 1; i <= 50; i++) {
        evaluator.multiply_inplace(ct, ct);
        evaluator.relinearize_inplace(ct, relin_keys);
        
        Plaintext temp;
        decryptor.decrypt(ct, temp);
        std::vector<double> vals;
        encoder.decode(temp, vals);
        double val = vals[0];
        
        gfn.rotate_seeds(val);
        boots++;
        
        encoder.encode(val, scale, temp);
        encryptor.encrypt(temp, ct);

        if (i % 10 == 0 || i == 50) {
            auto now = std::chrono::steady_clock::now();
            double elapsed = std::chrono::duration<double>(now - start).count();
            
            Plaintext check;
            decryptor.decrypt(ct, check);
            std::vector<double> decoded;
            encoder.decode(check, decoded);
            
            std::cout << "  Op " << std::setw(3) << i 
                      << " | val=" << std::fixed << std::setprecision(6) << decoded[0]
                      << " | cassini=" << std::fixed << std::setprecision(4) << gfn.get_min_cassini()
                      << " | boots=" << boots
                      << " | time=" << std::fixed << std::setprecision(1) << elapsed << "s\n";
        }
    }

    std::cout << "\n  SEAL + SpiralFHE: OK\n\n";
    return 0;
}
