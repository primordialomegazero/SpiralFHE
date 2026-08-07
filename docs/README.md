# SpiralFHE

Fully Homomorphic Encryption with self-optimizing bootstrap via Golden Fibonacci seed rotation and Cassini invariant verification.

## Architecture

```
src/core/constants.h       — Mathematical constants (φ, ψ)
src/fhe/fhe_core.h         — CKKS context, encrypt, decrypt, auto ring scaling
src/refresh/spiral_bootstrap.h — Complete bootstrap engine
tests/test_suite.cpp       — Test suite (8 tests)
```

## Quick Start

```bash
cd SpiralFHE
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

## Dependencies

- OpenFHE 1.5.1+
- GMP
- NTL
- AVX2-capable CPU

## Key Features

- Zero-circular-security bootstrap via seed rotation
- Cassini invariant for structural integrity verification
- Self-optimizing recursive fractal controller
- Side-channel defense (constant-time, memory barriers)
- N-configurable GF-N layers (template parameter)

## License

MIT
