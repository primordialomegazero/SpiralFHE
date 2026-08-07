# SpiralFHE

Fully Homomorphic Encryption with self-optimizing bootstrap via seed rotation and Cassini verification.

## Build

```bash
mkdir build && cd build
cmake .. && make -j$(nproc)
```

## Test

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

## Requirements

- OpenFHE 1.5+
- GMP, NTL
- AVX2 CPU

## License

MIT
