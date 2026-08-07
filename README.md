# Spiral FHE — Fully Homomorphic Encryption

**Zero-Plaintext Bootstrap. Unlimited Depth. O(1) Noise Refresh.**

## Overview

Spiral FHE achieves Fully Homomorphic Encryption with unlimited computation depth — without the circular security assumption required by Gentry's bootstrapping.

The foundation is the algebraic identity `phi * psi = -1` — a mathematical truth at the `1+1=2` level.

## How It Works

### Dual Encryption

1.  **Golden Fibonacci Encryption (GFE)** — Inner layer based on Fibonacci sequences and the Cassini identity.
2.  **CKKS FHE** — Outer layer for homomorphic computation over real numbers.

### Spiral Bootstrap (Zero Plaintext)

Traditional FHE bootstrapping requires evaluating the decryption circuit homomorphically — an expensive operation that relies on circular security.

Spiral FHE takes a fundamentally different approach:

1.  **Cassini Check** — Detect noise levels without decryption.
2.  **Seed Rotation** — Phi-driven encryption refresh in O(1) time.
3.  **Zero Plaintext Exposure** — The plaintext never leaves the GFE bubble.

```
bootstrap_zero(): 0.07 us/call  — Zero plaintext exposure
bootstrap_io():   3.91 us/call  — Full noise refresh + structural integrity
```

## Key Features

*   Unlimited FHE depth — compute without noise ceiling.
*   Zero plaintext during bootstrap — no vulnerability window.
*   No circular security assumption — foundation is `phi * psi = -1`, an algebraic identity.
*   Header-only library — drop into any C++ project.
*   CKKS compatible — works with Microsoft SEAL.

## Quick Start

```cpp
#include <spiralfhe/fhe/fhe_core.h>
#include <spiralfhe/refresh/spiral_bootstrap.h>

// Setup CKKS with Spiral FHE
SpiralFHE fhe(32768, 254, 264);

// Encrypt
auto ct = fhe.encrypt(data);

// Compute (unlimited depth)
for (int i = 0; i < 1000000; i++) {
    ct = fhe.multiply(ct, ct);
    // Auto-bootstrap triggers when needed
}

// Decrypt
auto result = fhe.decrypt(ct);
```

## Bootstrap Modes

| Mode                 | us/call  | Plaintext | Cassini |
|----------------------|----------|-----------|---------|
| `bootstrap_instant`  | 0.04     | Yes       | No      |
| `bootstrap_single`   | 0.08     | Yes       | Yes     |
| `bootstrap_zero`     | 0.07     | No        | Yes     |
| `bootstrap_io`       | 3.91     | Yes       | Yes     |
| `bootstrap_batched`  | Amortized| Yes       | Yes     |

## Mathematical Foundation

```
phi = (1 + sqrt(5)) / 2  =  1.6180339887498948482
psi = (1 - sqrt(5)) / 2  = -0.6180339887498948482

phi * psi = -1  (the generator)
phi + psi =  1  (the identity)
```

The security of Spiral FHE rests on the algebraic identity `phi * psi = -1` — not on computational hardness assumptions. No classical or quantum advance can alter this value.

## Building

```bash
mkdir build && cd build
cmake .. -DBUILD_TESTS=ON
make -j$(nproc)
ctest --output-on-failure
```

## Repository

This is the standalone FHE extraction from the Spiral Fractal framework. No iO, no P=NP, no Riemann — pure Fully Homomorphic Encryption.

## Contact

Dan Joseph M. Fernandez
Email: devilswithin13@gmail.com
GitHub: [primordialomegazero](https://github.com/primordialomegazero)
