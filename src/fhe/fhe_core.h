#pragma once
#include "../core/constants.h"
#include "../utils/logger.h"
#include "openfhe.h"
using namespace lbcrypto;

// ═══════════════════════════════════════════════════════════════════════════════
// FHE CORE — CKKS Fully Homomorphic Encryption Wrapper
//
// FORMAL PROOFS COVERED:
//   Theorem 2 (DualGate Projection): φ(a,b)·ψ(a,b) = a²+ab-b²
//   See: https://github.com/primordialomegazero/femmgFHE/blob/main/docs/FORMAL_PROOFS.md#theorem-2-dualgate-projection-identity
//
// ARCHITECTURE:
//   WHAT: Wraps OpenFHE CKKS for encrypted computation on DualGate {a,b} pairs.
//   WHY: DualGate enables φ/ψ projections that are algebraic conjugates,
//        making structural indistinguishability possible.
// ═══════════════════════════════════════════════════════════════════════════════
//
// Wraps OpenFHE CKKS scheme for encrypted computation.
//
// DualGate: A pair of CKKS ciphertexts {a, b} representing a value in R_φ².
// The two projections extract different circuit outputs:
//   φ(a,b) = a + b·φ  →  Circuit A output
//   ψ(a,b) = a + b·ψ  →  Circuit B output
//
// All logic gates (AND, OR, NAND, NOR, XOR, NOT) are derived from the
// fundamental NAND operation in the R_φ ring.
//
// ═══════════════════════════════════════════════════════════════════════════════

// [THEOREM 2] DualGate {a,b}: Two CKKS ciphertexts representing a value in R_φ².
// φ(a,b)=a+b·φ, ψ(a,b)=a+b·ψ — algebraic conjugates.
// See: https://github.com/primordialomegazero/femmgFHE/blob/main/docs/FORMAL_PROOFS.md#theorem-2-dualgate-projection-identity
// A pair of CKKS ciphertexts representing a DualGate value
struct DualGate { 
    Ciphertext<DCRTPoly> a;  // First component
    Ciphertext<DCRTPoly> b;  // Second component
};

// Holds the CKKS crypto context and key pair
struct SecureContext { 
    CryptoContext<DCRTPoly> cc;  // Crypto context
    KeyPair<DCRTPoly> kp;        // Public/secret key pair
};

// ═══════════════════════════════════════════════════════════════
// Create FHE context with specified parameters
//   rd: Ring dimension (2048, 4096, 8192, 16384, 32768)
//   dp: Multiplicative depth (60, 120, 200, 300)
// ═══════════════════════════════════════════════════════════════
inline SecureContext create_fhe_context(uint32_t rd, uint32_t dp, uint32_t batch_size = 0) {
    uint32_t N_batch = (batch_size > 0) ? batch_size : (rd / 16);
    CCParams<CryptoContextCKKSRNS> p;
    p.SetMultiplicativeDepth(dp);
    p.SetScalingModSize(50);
    p.SetBatchSize(N_batch);
    p.SetRingDim(rd);
    p.SetSecretKeyDist(UNIFORM_TERNARY);
    p.SetSecurityLevel(HEStd_NotSet);  // Allow custom RingDim
    
    auto cc = GenCryptoContext(p);
    cc->Enable(PKE);          // Public key encryption
    cc->Enable(KEYSWITCH);    // Key switching for multiplication
    cc->Enable(LEVELEDSHE);   // Leveled homomorphic encryption
    
    auto kp = cc->KeyGen();
    cc->EvalMultKeyGen(kp.secretKey);  // Generate relinearization keys
    
    Logger::info("FHE: RingDim=" + std::to_string(rd) + " Depth=" + std::to_string(dp) + " Batch=" + std::to_string(N_batch));
    return {cc, kp};
}

// ═══════════════════════════════════════════════════════════════
// Encrypt a value into a DualGate
// The 'b' component is initialized to encrypted zero
// ═══════════════════════════════════════════════════════════════
inline DualGate enc(SecureContext& sc, double v) {
    return {
        sc.cc->Encrypt(sc.kp.publicKey, sc.cc->MakeCKKSPackedPlaintext(std::vector<double>{v})),
        sc.cc->Encrypt(sc.kp.publicKey, sc.cc->MakeCKKSPackedPlaintext(std::vector<double>{0.0}))
    };
}

// ═══════════════════════════════════════════════════════════════
// Decrypt a CKKS ciphertext to a double value
// ═══════════════════════════════════════════════════════════════
inline double dec(SecureContext& sc, const Ciphertext<DCRTPoly>& c) {
    Plaintext pt;
    sc.cc->Decrypt(sc.kp.secretKey, c, &pt);
    return pt->GetCKKSPackedValue()[0].real();
}

// ═══════════════════════════════════════════════════════════════
// DualGate Projections
// φ(a,b) = a + b·φ — reveals Circuit A's view
// ψ(a,b) = a + b·ψ — reveals Circuit B's view
// ═══════════════════════════════════════════════════════════════
inline double phi_val(DualGate& g, SecureContext& sc) { 
    return dec(sc, g.a) + dec(sc, g.b) * PHI; 
}
inline double psi_val(DualGate& g, SecureContext& sc) { 
    return dec(sc, g.a) + dec(sc, g.b) * PSI; 
}

// ═══════════════════════════════════════════════════════════════
// NAND Gate — The universal gate in R_φ ring
// All other gates derived from this fundamental operation
// ═══════════════════════════════════════════════════════════════
inline DualGate nand_op(SecureContext& sc, DualGate& X, DualGate& Y) {
    // NAND(a,b) = (1 - a·b, -(a·b_s + a_s·b + b·b_s))
    auto a = sc.cc->EvalMult(X.a, Y.a);
    auto s = sc.cc->EvalAdd(
        sc.cc->EvalAdd(sc.cc->EvalMult(X.a, Y.b), sc.cc->EvalMult(X.b, Y.a)),
        sc.cc->EvalMult(X.b, Y.b)
    );
    auto one = sc.cc->MakeCKKSPackedPlaintext(std::vector<double>{1.0});
    auto neg = sc.cc->MakeCKKSPackedPlaintext(std::vector<double>{-1.0});
    return {sc.cc->EvalSub(one, a), sc.cc->EvalMult(neg, s)};
}

// Derived gates (all from NAND)
inline DualGate and_op(SecureContext& sc, DualGate& X, DualGate& Y) { 
    auto n = nand_op(sc, X, Y); 
    return nand_op(sc, n, n); 
}
inline DualGate or_op(SecureContext& sc, DualGate& X, DualGate& Y) { 
    auto nx = nand_op(sc, X, X), ny = nand_op(sc, Y, Y); 
    return nand_op(sc, nx, ny); 
}
inline DualGate not_op(SecureContext& sc, DualGate& X) { 
    return nand_op(sc, X, X); 
}
inline DualGate xor_op(SecureContext& sc, DualGate& X, DualGate& Y) {
    auto nx = not_op(sc, X), ny = not_op(sc, Y);
    auto x_and_ny = and_op(sc, X, ny), nx_and_y = and_op(sc, nx, Y);
    return or_op(sc, x_and_ny, nx_and_y);
}
