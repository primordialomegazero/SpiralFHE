#pragma once
#include <iostream>
#include <vector>
#include "openfhe.h"

using namespace lbcrypto;

struct SecureContext {
    CryptoContext<DCRTPoly> cc;
    KeyPair<DCRTPoly> kp;
};

inline uint32_t auto_ring_dim(uint32_t requested) {
    const uint32_t standard_dims[] = {1024, 2048, 4096, 8192, 16384, 32768, 65536, 131072};
    for (uint32_t dim : standard_dims) {
        if (dim >= requested) return dim;
    }
    return 131072;
}

inline SecureContext create_fhe_context(uint32_t ring_dim, uint32_t depth) {
    uint32_t actual_ring = auto_ring_dim(ring_dim);
    
    if (actual_ring != ring_dim) {
        std::cout << "  [INFO] Ring dimension: " << ring_dim 
                  << " -> " << actual_ring << "\n";
    }
    
    CCParams<CryptoContextCKKSRNS> params;
    params.SetMultiplicativeDepth(depth);
    params.SetScalingModSize(50);
    params.SetBatchSize(actual_ring / 2);
    params.SetRingDim(actual_ring);
    params.SetKeySwitchTechnique(HYBRID);
    params.SetScalingTechnique(FLEXIBLEAUTOEXT);

    CryptoContext<DCRTPoly> cc = GenCryptoContext(params);
    cc->Enable(PKE);
    cc->Enable(KEYSWITCH);
    cc->Enable(LEVELEDSHE);
    cc->Enable(ADVANCEDSHE);
    cc->Enable(MULTIPARTY);

    auto keys = cc->KeyGen();
    cc->EvalMultKeyGen(keys.secretKey);
    cc->EvalSumKeyGen(keys.secretKey);
    
    std::vector<int32_t> rot_indices;
    for (int32_t i = 1; i <= 2048; i *= 2) rot_indices.push_back(i);
    cc->EvalRotateKeyGen(keys.secretKey, rot_indices);

    SecureContext sc;
    sc.cc = cc;
    sc.kp = keys;
    return sc;
}

inline auto encrypt(const SecureContext& sc, double value) {
    auto pt = sc.cc->MakeCKKSPackedPlaintext(std::vector<double>{value});
    return sc.cc->Encrypt(sc.kp.publicKey, pt);
}

inline double decrypt(const SecureContext& sc, const Ciphertext<DCRTPoly>& ct) {
    Plaintext pt;
    sc.cc->Decrypt(sc.kp.secretKey, ct, &pt);
    return pt->GetCKKSPackedValue()[0].real();
}
