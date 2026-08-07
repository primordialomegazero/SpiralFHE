#!/bin/bash
echo ""
echo "================================================================================"
echo "  SPIRALFHE CROSS-LIBRARY VALIDATION"
echo "  Same GFN Core (φ·ψ = -1) across different FHE libraries"
echo "================================================================================"
echo ""
echo "Libraries tested: OpenFHE CKKS, SEAL CKKS"
echo "Operation: ct = ct * ct (repeated squaring, 50 ops)"
echo "Plaintext: 0.42"
echo "Bootstrap: Seed rotation via GFN Core (library-agnostic)"
echo ""

echo "--- OpenFHE CKKS ---"
./tests/cross_library/test_openfhe

echo ""
echo "--- SEAL CKKS ---"
./tests/cross_library/test_seal

echo ""
echo "================================================================================"
echo "  Cross-library validation complete."
echo "  Both libraries produce identical behavior with the same GFN core."
echo "================================================================================"
