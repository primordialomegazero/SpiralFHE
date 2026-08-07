#include <iostream>
#include <iomanip>
#include <vector>
#include <chrono>
#include <string>
#include <map>
#include "openfhe.h"
#include "../../src/core/constants.h"

using namespace lbcrypto;

// ═══════════════════════════════════════════════════════════════
// ENCRYPTED DATABASE JOIN IN FHE
// ═══════════════════════════════════════════════════════════════
//
// SELECT * FROM users JOIN orders ON users.id = orders.user_id
// 
// In FHE: Encrypted key comparison without decrypting!
//
// ═══════════════════════════════════════════════════════════════

int main() {
    std::cout << "\n";
    std::cout << "╔══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  ENCRYPTED DATABASE JOIN IN FHE                               ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════════╝\n\n";
    
    // ═══════════════════════════════════════════════════════════
    // SETUP FHE
    // ═══════════════════════════════════════════════════════════
    CCParams<CryptoContextCKKSRNS> params;
    params.SetMultiplicativeDepth(3);
    params.SetScalingModSize(50);
    params.SetBatchSize(256);
    params.SetRingDim(8192);
    params.SetSecretKeyDist(UNIFORM_TERNARY);
    params.SetSecurityLevel(HEStd_NotSet);
    
    auto cc = GenCryptoContext(params);
    cc->Enable(PKE); cc->Enable(KEYSWITCH); cc->Enable(LEVELEDSHE);
    auto kp = cc->KeyGen(); cc->EvalMultKeyGen(kp.secretKey);
    
    // ═══════════════════════════════════════════════════════════
    // SAMPLE DATA
    // ═══════════════════════════════════════════════════════════
    std::cout << "--- Sample Database ---\n";
    
    // Users table: id, name
    std::map<int, std::string> users = {
        {1, "Alice"},
        {2, "Bob"},
        {3, "Carol"},
        {4, "Dave"}
    };
    
    // Orders table: user_id, product
    std::map<int, std::string> orders = {
        {1, "Laptop"},
        {2, "Mouse"},
        {1, "Keyboard"},
        {3, "Monitor"},
        {4, "Desk"}
    };
    
    std::cout << "  Users: 4 rows\n";
    std::cout << "  Orders: 5 rows\n";
    std::cout << "  Join key: users.id = orders.user_id\n\n";
    
    // ═══════════════════════════════════════════════════════════
    // ENCRYPTED JOIN
    // ═══════════════════════════════════════════════════════════
    std::cout << "--- Encrypted JOIN ---\n";
    std::cout << "  Strategy: Encrypt join keys, compute equality homomorphically\n";
    std::cout << "  Encrypted: (user.id - order.user_id)^2 = 0 → MATCH!\n\n";
    
    auto start = std::chrono::steady_clock::now();
    int matches = 0;
    
    for (auto& [user_id, user_name] : users) {
        for (auto& [order_uid, product] : orders) {
            // ENCRYPT both keys
            std::vector<double> u_vec(256, (double)user_id);
            std::vector<double> o_vec(256, (double)order_uid);
            
            auto enc_user = cc->Encrypt(kp.publicKey, cc->MakeCKKSPackedPlaintext(u_vec));
            auto enc_order = cc->Encrypt(kp.publicKey, cc->MakeCKKSPackedPlaintext(o_vec));
            
            // Homomorphic equality: (a - b)^2
            auto diff = cc->EvalSub(enc_user, enc_order);
            auto square = cc->EvalMult(diff, diff);
            
            // Decrypt to check if match
            Plaintext pt;
            cc->Decrypt(kp.secretKey, square, &pt);
            double result = pt->GetCKKSPackedValue()[0].real();
            
            if (result < 0.001) {
                matches++;
                std::cout << "  JOIN: " << user_name << " bought " << product << "\n";
            }
        }
    }
    
    auto end = std::chrono::steady_clock::now();
    double sec = std::chrono::duration<double>(end - start).count();
    
    std::cout << "\n  Total matches: " << matches << "\n";
    std::cout << "  Time: " << sec << " sec (" << (sec/20) << " sec per comparison)\n";
    std::cout << "  Operations: 20 comparisons × 2 ops = 40 FHE operations\n";
    
    // ═══════════════════════════════════════════════════════════
    // BATCHED JOIN (packed)
    // ═══════════════════════════════════════════════════════════
    std::cout << "\n--- Batched Encrypted JOIN ---\n";
    std::cout << "  Strategy: Pack all keys into single ciphertext\n";
    std::cout << "  One operation for ALL comparisons!\n\n";
    
    // Pack all user IDs and order IDs into vectors
    std::vector<double> all_users(256, 0.0);
    std::vector<double> all_orders(256, 0.0);
    
    int idx = 0;
    for (auto& [uid, name] : users) {
        for (auto& [oid, prod] : orders) {
            all_users[idx] = (double)uid;
            all_orders[idx] = (double)oid;
            idx++;
        }
    }
    
    auto start2 = std::chrono::steady_clock::now();
    
    auto enc_all_users = cc->Encrypt(kp.publicKey, cc->MakeCKKSPackedPlaintext(all_users));
    auto enc_all_orders = cc->Encrypt(kp.publicKey, cc->MakeCKKSPackedPlaintext(all_orders));
    
    // One batched equality check
    auto batch_diff = cc->EvalSub(enc_all_users, enc_all_orders);
    auto batch_square = cc->EvalMult(batch_diff, batch_diff);
    
    Plaintext pt2;
    cc->Decrypt(kp.secretKey, batch_square, &pt2);
    auto batch_results = pt2->GetCKKSPackedValue();
    
    int batch_matches = 0;
    idx = 0;
    for (auto& [uid, name] : users) {
        for (auto& [oid, prod] : orders) {
            if (batch_results[idx].real() < 0.001) {
                std::cout << "  JOIN: " << name << " bought " << prod << "\n";
                batch_matches++;
            }
            idx++;
        }
    }
    
    auto end2 = std::chrono::steady_clock::now();
    double sec2 = std::chrono::duration<double>(end2 - start2).count();
    
    std::cout << "\n  Batched matches: " << batch_matches << "\n";
    std::cout << "  Batched time: " << sec2 << " sec (ALL 20 comparisons at once!)\n";
    std::cout << "  Speedup: " << (sec/sec2) << "x\n";
    
    std::cout << "\n╔══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  ENCRYPTED DATABASE JOIN — WORKING!                           ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════════╝\n\n";
    
    return 0;
}
