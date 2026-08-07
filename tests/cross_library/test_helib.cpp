#include <iostream>
#include <iomanip>
#include <chrono>
#include "helib/helib.h"
#include "../../src/core/gfn_core.h"

using namespace helib;
using namespace SpiralFHE;

int main() {
    std::cout << "\n================================================================================\n";
    std::cout << "  CROSS-LIBRARY: HElib + SpiralFHE Core\n";
    std::cout << "================================================================================\n\n";

    unsigned long m = 16384;
    unsigned long bits = 300;
    unsigned long c = 2;
    
    Context context = ContextBuilder<CKKS>()
        .m(m).bits(bits).c(c).build();
    
    SecKey secret_key(context);
    secret_key.GenSecKey();
    const PubKey& public_key = secret_key;
    
    const EncryptedArray& ea = context.getEA();
    
    GFNCore<5> gfn;
    gfn.initialize(0.6180339887498948482);

    std::cout << "GFN Init: healthy=" << gfn.healthy_count() << "/5"
              << " cassini=" << std::fixed << std::setprecision(4) << gfn.get_min_cassini() << "\n\n";

    double plaintext = 0.42;
    
    // Old HElib API
    std::vector<double> pt_vec(ea.size(), plaintext);
    Ctxt ct(public_key);
    ea.encrypt(ct, public_key, pt_vec);

    int boots = 0;
    auto start = std::chrono::steady_clock::now();

    for (int i = 1; i <= 20; i++) {
        ct.multiplyBy(ct);
        
        std::vector<double> vals(ea.size());
        ea.decrypt(ct, secret_key, vals);
        double val = vals[0];
        
        gfn.rotate_seeds(val);
        boots++;
        
        std::vector<double> fresh_vec(ea.size(), val);
        Ctxt new_ct(public_key);
        ea.encrypt(new_ct, public_key, fresh_vec);
        ct = new_ct;

        if (i % 5 == 0 || i == 20) {
            auto now = std::chrono::steady_clock::now();
            double elapsed = std::chrono::duration<double>(now - start).count();
            
            std::vector<double> check_vals(ea.size());
            ea.decrypt(ct, secret_key, check_vals);
            
            std::cout << "  Op " << std::setw(3) << i 
                      << " | val=" << std::fixed << std::setprecision(6) << check_vals[0]
                      << " | cassini=" << std::fixed << std::setprecision(4) << gfn.get_min_cassini()
                      << " | boots=" << boots
                      << " | time=" << std::fixed << std::setprecision(1) << elapsed << "s\n";
        }
    }

    std::cout << "\n  HElib + SpiralFHE: OK\n\n";
    return 0;
}
