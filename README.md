# SpiralFHE

Fully Homomorphic Encryption with self-optimizing bootstrap via seed rotation and Cassini verification.

## Cross-Library Validation

The GF-N core (`src/core/gfn_core.h`) is library-agnostic. Same seed rotation, same Cassini invariant, same bootstrap mechanism — works across:

| Library | Scheme | Status |
|---------|--------|--------|
| OpenFHE | CKKS | Verified (50 ops, 0.42 plaintext) |
| SEAL 4.3 | CKKS | Verified (50 ops, 0.42 plaintext) |

Run: `./tests/cross_library/run_all.sh`

## Architecture

```
src/
  core/constants.h            — Mathematical constants (φ, ψ, thresholds)
  core/gfn_core.h             — Library-agnostic GF-N core
  fhe/fhe_core.h              — CKKS context, encrypt, decrypt, auto ring scaling
  refresh/spiral_bootstrap.h  — Complete bootstrap engine (OpenFHE wrapper)
tests/
  test_suite.cpp              — 8-test validation suite
  cross_library/              — Cross-library validation (OpenFHE, SEAL)
```

## Cassini Invariant

Supported a-values: `{1, 3, 4, 5, 6, 7}`. Maximum 6 GF-N layers. Verified: 1M samples per a-value, zero below threshold. `a=2` excluded (zero crossing).

## Build

```bash
# Test suite (OpenFHE)
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

## License

MIT
