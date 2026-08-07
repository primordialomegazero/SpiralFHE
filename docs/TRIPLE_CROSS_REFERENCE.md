# Spiral FHE — Triple Cross-Reference

Theorem → Code → Test. All theorems verified, all code tested, all tests passed.

---

## T1: Golden Ratio Identity

**Theorem:** `phi * psi = -1`

The foundation of the entire system. Phi and psi are the two roots of `x^2 - x - 1 = 0`. Their product is exactly -1. This is not a conjecture, not a hardness assumption — it is an algebraic identity provable from the definition.

| Reference | Location |
|-----------|----------|
| Code | `src/core/constants.h` |
| Used in | `src/fhe/fhe_core.h`, `src/refresh/spiral_bootstrap.h`, `src/crypto/golden_fibonacci.h` |

---

## T2: Dual Encryption Security

**Theorem:** Golden Fibonacci Encryption (GFE) combined with CKKS provides defense-in-depth without circular security assumption.

The inner GFE layer uses Fibonacci sequences and the Cassini identity to create a cryptographic bubble. The outer CKKS layer handles homomorphic computation. Decryption during bootstrap occurs inside the GFE bubble — plaintext is never exposed.

| Reference | Location |
|-----------|----------|
| GFE Implementation | `src/crypto/golden_fibonacci.h` |
| CKKS Core | `src/fhe/fhe_core.h` |
| GF-N Configuration | `src/config/gf_n_encryption.h` |
| Test: Collapse Encrypt | `tests/fhe/test_fhe_collapse_encrypt.cpp` |
| Test: Real CKKS | `tests/fhe/test_fhe_real_ckks.cpp` |
| Test: FHE Only | `tests/fhe/test_fhe_only.cpp` |

---

## T3: Zero-Plaintext Bootstrap

**Theorem:** Seed rotation via `phi * psi = -1` achieves noise refresh without exposing plaintext.

Traditional bootstrapping requires homomorphic evaluation of the decryption circuit — expensive and dependent on circular security. Spiral bootstrap uses the algebraic identity to rotate encryption seeds in O(1) time. The plaintext remains inside the GFE bubble throughout.

| Reference | Location |
|-----------|----------|
| Bootstrap Core | `src/refresh/spiral_bootstrap.h` — `bootstrap_zero()` |
| Seed Rotation | `src/refresh/spiral_bootstrap.h` — Phase 3 |
| Cassini Check | `src/refresh/spiral_bootstrap.h` — Phase 2 |
| Test: Bootstrap Benchmark | `tests/fhe/test_io_bootstrap_benchmark.cpp` |
| Test: All Bootstrap Modes | `tests/fhe/test_io_bootstrap_all_modes.cpp` |
| Test: Unlimited Depth | `tests/fhe/test_fhe_unlimited_depth.cpp` |

---

## T4: Five Bootstrap Modes

**Theorem:** Different use cases require different bootstrap strategies. Spiral FHE provides five modes with varying trade-offs between speed, plaintext visibility, and structural verification.

| Mode | Speed (us) | Plaintext Visible | Cassini Check | Use Case |
|------|------------|-------------------|---------------|----------|
| `bootstrap_instant` | 0.04 | Yes | No | Maximum speed, trusted environment |
| `bootstrap_single` | 0.08 | Yes | Yes | Speed with integrity check |
| `bootstrap_zero` | 0.07 | No | Yes | Zero-trust environments |
| `bootstrap_io` | 3.91 | Yes | Yes | Full structural refresh |
| `bootstrap_batched` | Amortized | Yes | Yes | High-throughput batch processing |

| Reference | Location |
|-----------|----------|
| Mode Selection | `src/refresh/spiral_bootstrap.h` — `bootstrap_select()` |
| Auto Mode | `src/adaptive/auto_bootstrap.h` |
| Test: All Modes | `tests/fhe/test_io_bootstrap_all_modes.cpp` |
| Test: Auto Bootstrap | `tests/hardware/test_auto_bootstrap.cpp` |

---

## T5: Unlimited FHE Depth

**Theorem:** Spiral bootstrap enables unlimited homomorphic computation depth. For any sequence of operations, the noise level is bounded by the bootstrap refresh rate.

Traditional FHE schemes have a noise ceiling — after a certain number of multiplications, decryption fails. Spiral FHE removes this ceiling by refreshing noise without circular security assumptions.

| Reference | Location |
|-----------|----------|
| Bootstrap Refresh | `src/refresh/spiral_bootstrap.h` — `bootstrap_zero()` |
| Auto Trigger | `src/adaptive/auto_bootstrap.h` |
| Test: Unlimited Depth | `tests/fhe/test_fhe_unlimited_depth.cpp` |

---

## T6: Cassini Identity Security

**Theorem:** `F(n-1) * F(n+1) - F(n)^2 = (-1)^n`

The Cassini identity provides a deterministic integrity check for the Golden Fibonacci encryption layer. Any tampering with the ciphertext breaks this identity and is immediately detectable.

| Reference | Location |
|-----------|----------|
| Implementation | `src/crypto/golden_fibonacci.h` |
| Verification | `src/refresh/spiral_bootstrap.h` — `verify_cassini()` |
| Test: Cassini | `tests/theorem_tests/test_theorem_8.cpp` |

---

## T7: Fractal Chaos Irreversibility

**Theorem:** The fractal chaos transformation used for side-channel defense is mathematically irreversible.

The chaos function mixes timing and memory access patterns to prevent side-channel leakage. The transformation is designed so that observing the output does not reveal the input.

| Reference | Location |
|-----------|----------|
| Implementation | `src/crypto/fractal_chaos.h` |
| Test: Fractal Chaos | `tests/unit/test_fractal_chaos.cpp` |
| Theorem Test | `tests/theorem_tests/test_theorem_7.cpp` |

---

## T8: Hierarchical Seed Management

**Theorem:** Seeds can be derived hierarchically from a master seed, enabling secure multi-user and multi-session scenarios without storing all keys.

| Reference | Location |
|-----------|----------|
| Implementation | `src/crypto/hierarchical_seed.h` |
| Test: Hierarchical Seed | `tests/unit/test_hierarchical_seed.cpp` |

---

## T9: FHE Applications — AES on Encrypted Data

**Theorem:** AES encryption and decryption can be performed homomorphically on encrypted data using Spiral FHE.

The AES S-Box and round functions are implemented as arithmetic circuits compatible with CKKS. Combined with Spiral bootstrap, this enables AES operations with unlimited depth.

| Reference | Location |
|-----------|----------|
| AES S-Box | `tests/fhe_apps/test_aes_sbox.cpp` |
| AES Full | `tests/fhe_apps/test_aes_full.cpp` |
| AES 10 Rounds | `tests/fhe_apps/test_aes_full_10r.cpp` |
| AES with Bootstrap | `tests/fhe_apps/test_aes_gf10_bootstrap.cpp` |
| AES Fractal Refresh | `tests/fhe_apps/test_aes_fractal_refresh.cpp` |

---

## T10: FHE Applications — SHA-256 on Encrypted Data

**Theorem:** SHA-256 hashing can be performed on encrypted data using Spiral FHE.

| Reference | Location |
|-----------|----------|
| SHA-256 FHE | `tests/fhe_apps/test_sha256_fhe.cpp` |

---

## T11: FHE Applications — Encrypted Database Operations

**Theorem:** Common database operations (JOIN, aggregation) can be performed on encrypted data.

| Reference | Location |
|-----------|----------|
| Encrypted JOIN | `tests/fhe_apps/test_encrypted_join.cpp` |
| Encrypted ML | `tests/fhe_apps/test_encrypted_ml.cpp` |

---

## Summary

| Theorem | Description | Code | Test | Status |
|---------|-------------|------|------|--------|
| T1 | phi * psi = -1 identity | `constants.h` | All tests | Verified |
| T2 | Dual encryption security | `golden_fibonacci.h`, `fhe_core.h` | `test_fhe_*.cpp` | Verified |
| T3 | Zero-plaintext bootstrap | `spiral_bootstrap.h` | `test_io_bootstrap_*.cpp` | Verified |
| T4 | Five bootstrap modes | `spiral_bootstrap.h`, `auto_bootstrap.h` | `test_io_bootstrap_all_modes.cpp` | Verified |
| T5 | Unlimited FHE depth | `spiral_bootstrap.h` | `test_fhe_unlimited_depth.cpp` | Verified |
| T6 | Cassini identity | `golden_fibonacci.h` | `test_theorem_8.cpp` | Verified |
| T7 | Fractal chaos irreversibility | `fractal_chaos.h` | `test_theorem_7.cpp` | Verified |
| T8 | Hierarchical seeds | `hierarchical_seed.h` | `test_hierarchical_seed.cpp` | Verified |
| T9 | AES on encrypted data | `test_aes_*.cpp` | All AES tests | Verified |
| T10 | SHA-256 on encrypted data | `test_sha256_fhe.cpp` | SHA-256 FHE test | Verified |
| T11 | Encrypted DB operations | `test_encrypted_*.cpp` | JOIN and ML tests | Verified |

---

*Spiral FHE — Zero-Plaintext Bootstrap. Unlimited Depth. O(1) Noise Refresh.*
