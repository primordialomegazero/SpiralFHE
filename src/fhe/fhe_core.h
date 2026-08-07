#pragma once
#include "../core/constants.h"
#include "../utils/logger.h"
#include "openfhe.h"
using namespace lbcrypto;

// ==========================================
// FHE CORE — CKKS Homomorphic Encryption with Logic Gates
// ==========================================
//
// THEOREM (Homomorphic Logic Gates):
//   Any Boolean circuit can be evaluated on encrypted data using
//   only NAND gates. Given encrypted inputs Enc(a) and Enc(b):
//     NAND(a,b) = 1 - a*b
//   All other gates (AND, OR, XOR, NOT) are derived from NAND.
//
//   In Spiral FHE, values are represented as DualSlot {a, b}
//   pairs in CKKS ciphertext space. The two components enable
//   encrypted computation while preserving the mathematical
//   structure needed for bootstrap operations.
//
//   CKKS security is based on the Ring Learning With Errors
//   (RLWE) problem. Combined with GF-N inner encryption,
//   Spiral FHE provides defense-in-depth without circular
//   security assumptions.
//
// USED IN:
//   - spiral_bootstrap.h:  Bootstrap refresh of CKKS ciphertexts
//   - All FHE applications (AES, SHA-256, encrypted ML)
//
// CROSS-REFERENCE:
//   Theorem T1: phi * psi = -1  (constants.h)
//   Theorem T2: Dual encryption security (gf_n_encryption.h)
//   Theorem T3: Zero-plaintext bootstrap (spiral_bootstrap.h)
//   Theorem T9: AES on encrypted data (test_aes_sbox.cpp)
// ==========================================

// DualSlot: A pair of CKKS ciphertexts representing an encrypted value.
// The two components provide the mathematical structure for
// homomorphic computation and bootstrap operations.
struct DualSlot {
    Ciphertext<DCRTPoly> a;
    Ciphertext<DCRTPoly> b;
};

// Holds the CKKS crypto context and key pair
struct SecureContext {
    CryptoContext<DCRTPoly> cc;
    KeyPair<DCRTPoly> kp;
};

// ==========================================
// Create CKKS FHE context
//
//   ring_dim:   Ring dimension (4096, 8192, 16384, 32768)
//   depth:      Multiplicative depth before bootstrap
//   batch_size: Number of slots (default: ring_dim / 16)
// ==========================================
inline SecureContext create_fhe_context(uint32_t ring_dim, uint32_t depth,
                                        uint32_t batch_size = 0) {
    uint32_t slots = (batch_size > 0) ? batch_size : (ring_dim / 16);

    CCParams<CryptoContextCKKSRNS> params;
    params.SetMultiplicativeDepth(depth);
    params.SetScalingModSize(50);
    params.SetBatchSize(slots);
    params.SetRingDim(ring_dim);
    params.SetSecretKeyDist(UNIFORM_TERNARY);
    params.SetSecurityLevel(HEStd_NotSet);

    auto cc = GenCryptoContext(params);
    cc->Enable(PKE);
    cc->Enable(KEYSWITCH);
    cc->Enable(LEVELEDSHE);

    auto kp = cc->KeyGen();
    cc->EvalMultKeyGen(kp.secretKey);

    Logger::info("CKKS: ring_dim=" + std::to_string(ring_dim) +
                 " depth=" + std::to_string(depth) +
                 " slots=" + std::to_string(slots));
    return {cc, kp};
}

// ==========================================
// Encrypt a value into a DualSlot
// The 'b' component is initialized to encrypted zero
// ==========================================
inline DualSlot encrypt(SecureContext& sc, double value) {
    return {
        sc.cc->Encrypt(sc.kp.publicKey,
            sc.cc->MakeCKKSPackedPlaintext(std::vector<double>{value})),
        sc.cc->Encrypt(sc.kp.publicKey,
            sc.cc->MakeCKKSPackedPlaintext(std::vector<double>{0.0}))
    };
}

// ==========================================
// Decrypt a CKKS ciphertext to a double
// ==========================================
inline double decrypt(SecureContext& sc, const Ciphertext<DCRTPoly>& ct) {
    Plaintext pt;
    sc.cc->Decrypt(sc.kp.secretKey, ct, &pt);
    return pt->GetCKKSPackedValue()[0].real();
}

// ==========================================
// NAND Gate — The universal gate
//
//   NAND(a,b) = 1 - a*b
//
// All other gates are derived from NAND.
// This is the fundamental building block for
// encrypted circuit evaluation.
// ==========================================
inline DualSlot nand_op(SecureContext& sc, DualSlot& X, DualSlot& Y) {
    auto a = sc.cc->EvalMult(X.a, Y.a);
    auto s = sc.cc->EvalAdd(
        sc.cc->EvalAdd(
            sc.cc->EvalMult(X.a, Y.b),
            sc.cc->EvalMult(X.b, Y.a)
        ),
        sc.cc->EvalMult(X.b, Y.b)
    );
    auto one = sc.cc->MakeCKKSPackedPlaintext(std::vector<double>{1.0});
    auto neg = sc.cc->MakeCKKSPackedPlaintext(std::vector<double>{-1.0});
    return {
        sc.cc->EvalSub(one, a),
        sc.cc->EvalMult(neg, s)
    };
}

// ==========================================
// AND Gate — Derived from NAND
//   AND(a,b) = NAND(NAND(a,b), NAND(a,b))
// ==========================================
inline DualSlot and_op(SecureContext& sc, DualSlot& X, DualSlot& Y) {
    auto n = nand_op(sc, X, Y);
    return nand_op(sc, n, n);
}

// ==========================================
// OR Gate — Derived from NAND
//   OR(a,b) = NAND(NAND(a,a), NAND(b,b))
// ==========================================
inline DualSlot or_op(SecureContext& sc, DualSlot& X, DualSlot& Y) {
    auto nx = nand_op(sc, X, X);
    auto ny = nand_op(sc, Y, Y);
    return nand_op(sc, nx, ny);
}

// ==========================================
// NOT Gate — Derived from NAND
//   NOT(a) = NAND(a, a)
// ==========================================
inline DualSlot not_op(SecureContext& sc, DualSlot& X) {
    return nand_op(sc, X, X);
}

// ==========================================
// XOR Gate — Derived from NAND
//   XOR(a,b) = OR(AND(a, NOT(b)), AND(NOT(a), b))
// ==========================================
inline DualSlot xor_op(SecureContext& sc, DualSlot& X, DualSlot& Y) {
    auto nx = not_op(sc, X);
    auto ny = not_op(sc, Y);
    auto x_and_ny = and_op(sc, X, ny);
    auto nx_and_y = and_op(sc, nx, Y);
    return or_op(sc, x_and_ny, nx_and_y);
}
