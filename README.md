# SpiralFHE

Fully Homomorphic Encryption with self-optimizing bootstrap via seed rotation and Cassini verification.

## Architecture

```
src/
  core/constants.h            — Mathematical constants (φ, ψ, thresholds)
  fhe/fhe_core.h              — CKKS context creation, encrypt, decrypt, auto ring scaling
  refresh/spiral_bootstrap.h  — Complete bootstrap engine
tests/
  test_suite.cpp              — 8-test validation suite
  test_proof_verification.cpp — Cassini invariant verification (a-values)
```

## Cassini Invariant

The Cassini invariant uses supported a-values: `{1, 3, 4, 5, 6, 7}`.

`a=2` is excluded — it has a zero crossing at `s=0.374653...` where the invariant collapses to zero. All other a-values maintain `C > 0.1` for all seeds in `[0,1)`. Verified with 1,000,000 samples per a-value, zero below threshold.

Maximum GF-N layers: 6 (one per supported a-value).

## Bootstrap Rate

Bounded by `1/ceil((D-2)/3)` where D is CKKS depth. For D=32: max 10% bootstrap rate. For D=64: max ~4.8%.

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

| Parameter | Default | Max | Description |
|-----------|---------|-----|-------------|
| `GFNLayers` | 5 | 6 | Number of GF-N layers (a-values from {1,3,4,5,6,7}) |
| `HistorySize` | 30 | — | Controller history window |

## Dependencies

- OpenFHE 1.5+
- GMP, NTL
- AVX2 CPU

## License

MIT
