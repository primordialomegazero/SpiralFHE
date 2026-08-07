# SpiralFHE

Fully Homomorphic Encryption with unlimited depth via GF-N seed rotation and Cassini invariant verification. No circular security assumption. Self-optimizing bootstrap controller.

## Architecture

```
src/
  core/constants.h            — Mathematical constants (φ, ψ, thresholds)
  fhe/fhe_core.h              — CKKS context creation, encrypt, decrypt, auto ring scaling
  refresh/spiral_bootstrap.h  — Complete bootstrap engine
tests/
  test_suite.cpp              — 8-test validation suite
```

## Core Components

### GF-N Encryption
Golden Fibonacci N-layer encryption. Each layer encodes state using φ-weighted values. Layer seeds derived via `seed_{i+1} = fmod(seed_i * φ + 0.618, 1.0)`.

### Cassini Invariant
Structural integrity check: `|(y1 + (i+1)*φ) * (y2 + (i+1)*ψ) + 1| > 0.1` per layer. Based on the algebraic identity `φ * ψ = -1`. Verification does not require decryption.

### Seed Rotation (Bootstrap)
Forward-secure state refresh: `new_seed = fmod(cached_seed * φ + gf_state * 0.001, 1.0)`. All layer states recomputed from new seed. Previous seeds cannot be recovered.

### Recursive Fractal Controller
Three-level self-optimizing controller:

- **Level 1 (Controller):** Computes integrated PHI from Cassini health and CKKS level. Decides when to bootstrap.
- **Level 2 (Meta-Controller):** Observes PHI stability over time. Adjusts Cassini threshold and learning rate. Eliminates manual tuning.
- **Level 3 (Fractal Detector):** Detects self-similarity between Level 1 and Level 2. Signals when optimal parameters are reached.

### Bootstrap Decision (Cross-Monitoring)
Four independent conditions, any triggers refresh:
1. CKKS level below learned minimum
2. Cassini below adjusted threshold
3. Layer health degraded
4. Emergency: Cassini below critical floor

### Side-Channel Defense
Constant-time operations, memory barriers (mfence), stack prefaulting. Prevents timing and power analysis on the bootstrap path.

## Dependencies

- OpenFHE 1.5+
- GMP
- NTL
- GCC 11+ with AVX2 support

## Build and Test

```bash
g++ -std=c++17 -O2 \
    -I/usr/local/openfhe/include/openfhe/pke \
    -I/usr/local/openfhe/include/openfhe/core \
    -I/usr/local/openfhe/include/openfhe/binfhe \
    -I./src \
    -o test_suite tests/test_suite.cpp \
    -L/usr/local/openfhe/lib \
    -lOPENFHEpke -lOPENFHEcore -lOPENFHEbinfhe \
    -lntl -lgmp \
    -Wl,-rpath,/usr/local/openfhe/lib \
    -mavx2 -mfma

./test_suite
```

## Template Parameters

| Parameter | Default | Description |
|-----------|---------|-------------|
| `GFNLayers` | 5 | Number of GF-N encryption layers |
| `HistorySize` | 30 | Controller history window size |

```cpp
CompleteBootstrap<5, 30> cb;   // Default: 5 layers, 30 history
CompleteBootstrap<3, 20> cb;   // Lightweight: 3 layers, 20 history
CompleteBootstrap<7, 50> cb;   // High security: 7 layers, 50 history
```

## Test Suite

| Test | Operation | Description |
|------|-----------|-------------|
| Add Constants | `ct + 0.5` | 20 additions with constant |
| Multiply Constants | `ct * 0.5` | 20 multiplications with constant |
| Rotations | `EvalRotate(ct, 1)` | 10 cyclic shifts |
| Sum All Slots | `EvalSum(ct)` | 10 slot summations |
| Add/Sub Cycles | `(ct + 0.5) - 0.5` | 20 value preservation cycles |
| Chained Ops | `Add → Mult → Sub` | 15 compound operations |
| Squaring | `ct * ct` | 15 stress test squarings |
| Value Preservation | `(x + 0.5) - 0.5` | 30 cycles at value 24325.00 |

## License

MIT

## Contact

Dan Joseph M. Fernandez
Email: devilswithin13@gmail.com
GitHub: primordialomegazero
