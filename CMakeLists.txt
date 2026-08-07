cmake_minimum_required(VERSION 3.16)
project(SpiralFHE VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ==========================================
# Spiral FHE — Standalone Fully Homomorphic Encryption
# Foundation: phi * psi = -1 -> Zero-Plaintext Bootstrap
# ==========================================

# --- Dependencies --------------------------
find_package(OpenSSL REQUIRED)
find_package(SEAL REQUIRED)

# --- Spiral FHE Library --------------------
add_library(spiralfhe INTERFACE)

target_include_directories(spiralfhe INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(spiralfhe INTERFACE
    OpenSSL::OpenSSL
    SEAL::seal
)

# All Spiral FHE components are header-only for maximum portability.
# Include directories:
#   src/core/          — phi, psi constants
#   src/crypto/        — Golden Fibonacci, Fractal Chaos, Hierarchical Seed
#   src/fhe/           — CKKS FHE Core
#   src/refresh/       — Spiral Bootstrap (zero-plaintext)
#   src/config/        — GF-N Encryption configuration
#   src/adaptive/      — Auto Bootstrap state machine
#   src/utils/         — Safe arithmetic

# --- Tests ---------------------------------
option(BUILD_TESTS "Build Spiral FHE tests" ON)

if(BUILD_TESTS)
    enable_testing()

    # FHE Core tests
    add_executable(test_fhe_real_ckks tests/fhe/test_fhe_real_ckks.cpp)
    target_link_libraries(test_fhe_real_ckks spiralfhe)
    add_test(NAME test_fhe_real_ckks COMMAND test_fhe_real_ckks)

    add_executable(test_fhe_collapse_encrypt tests/fhe/test_fhe_collapse_encrypt.cpp)
    target_link_libraries(test_fhe_collapse_encrypt spiralfhe)
    add_test(NAME test_fhe_collapse_encrypt COMMAND test_fhe_collapse_encrypt)

    add_executable(test_fhe_only tests/fhe/test_fhe_only.cpp)
    target_link_libraries(test_fhe_only spiralfhe)
    add_test(NAME test_fhe_only COMMAND test_fhe_only)

    add_executable(test_fhe_unlimited_depth tests/fhe/test_fhe_unlimited_depth.cpp)
    target_link_libraries(test_fhe_unlimited_depth spiralfhe)
    add_test(NAME test_fhe_unlimited_depth COMMAND test_fhe_unlimited_depth)

    # Bootstrap tests
    add_executable(test_spiral_bootstrap tests/unit/test_spiral_bootstrap.cpp)
    target_link_libraries(test_spiral_bootstrap spiralfhe)
    add_test(NAME test_spiral_bootstrap COMMAND test_spiral_bootstrap)

    add_executable(test_bootstrap_benchmark tests/fhe/test_io_bootstrap_benchmark.cpp)
    target_link_libraries(test_bootstrap_benchmark spiralfhe)
    add_test(NAME test_bootstrap_benchmark COMMAND test_bootstrap_benchmark)

    add_executable(test_bootstrap_all_modes tests/fhe/test_io_bootstrap_all_modes.cpp)
    target_link_libraries(test_bootstrap_all_modes spiralfhe)
    add_test(NAME test_bootstrap_all_modes COMMAND test_bootstrap_all_modes)

    # Auto Bootstrap
    add_executable(test_auto_bootstrap tests/hardware/test_auto_bootstrap.cpp)
    target_link_libraries(test_auto_bootstrap spiralfhe)
    add_test(NAME test_auto_bootstrap COMMAND test_auto_bootstrap)

    # FHE Applications
    add_executable(test_aes_sbox tests/fhe_apps/test_aes_sbox.cpp)
    target_link_libraries(test_aes_sbox spiralfhe)
    add_test(NAME test_aes_sbox COMMAND test_aes_sbox)

    add_executable(test_sha256_fhe tests/fhe_apps/test_sha256_fhe.cpp)
    target_link_libraries(test_sha256_fhe spiralfhe)
    add_test(NAME test_sha256_fhe COMMAND test_sha256_fhe)

    # Crypto unit tests
    add_executable(test_fractal_chaos tests/unit/test_fractal_chaos.cpp)
    target_link_libraries(test_fractal_chaos spiralfhe)
    add_test(NAME test_fractal_chaos COMMAND test_fractal_chaos)

    add_executable(test_hierarchical_seed tests/unit/test_hierarchical_seed.cpp)
    target_link_libraries(test_hierarchical_seed spiralfhe)
    add_test(NAME test_hierarchical_seed COMMAND test_hierarchical_seed)

    # Theorem tests
    add_executable(test_theorem_7 tests/theorem_tests/test_theorem_7.cpp)
    target_link_libraries(test_theorem_7 spiralfhe)
    add_test(NAME test_theorem_7 COMMAND test_theorem_7)

    add_executable(test_theorem_8 tests/theorem_tests/test_theorem_8.cpp)
    target_link_libraries(test_theorem_8 spiralfhe)
    add_test(NAME test_theorem_8 COMMAND test_theorem_8)

    message(STATUS "Tests enabled: 13 test targets")
endif()

# --- Install -------------------------------
install(DIRECTORY src/ DESTINATION include/spiralfhe
    FILES_MATCHING PATTERN "*.h"
)
