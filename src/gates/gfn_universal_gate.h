#pragma once
#include <cmath>
#include <cstdint>
#include <vector>
#include "../refresh/spiral_bootstrap_zerodecrypt.h"

namespace SpiralFHE {

class GFNGateCore {
public:
    static constexpr double PHI = SpiralConstants::PHI;
    static constexpr double PSI = SpiralConstants::PSI;
    
    static double canonicalize(double value, int depth = 3) {
        double current = value;
        for (int d = 0; d < depth; d++) {
            double encoded, collapsed;
            if (d % 2 == 0) {
                encoded = current * PHI;
                collapsed = std::fabs(encoded * PSI);
            } else {
                encoded = current * PSI;
                collapsed = std::fabs(encoded * PHI);
            }
            current = collapsed;
        }
        return std::fabs(current);
    }
    
    static bool to_bool(double value) { return std::fabs(value) >= 0.5; }
    static double to_double(bool b) { return b ? 1.0 : 0.0; }
};

class GFNGate {
private:
    ZeroDecryptBootstrap* bootstrap;
    
public:
    GFNGate() : bootstrap(nullptr) {}
    void attach_bootstrap(ZeroDecryptBootstrap* b) { bootstrap = b; }
    bool has_bootstrap() const { return bootstrap != nullptr && bootstrap->is_healthy(); }
    
    double NAND(double a, double b) { return GFNGateCore::canonicalize(1.0 - a * b); }
    double AND(double a, double b) { return GFNGateCore::canonicalize(a * b); }
    double OR(double a, double b) { return GFNGateCore::canonicalize(a + b - a * b); }
    double NOT(double a) { return GFNGateCore::canonicalize(1.0 - a); }
    double XOR(double a, double b) { return GFNGateCore::canonicalize(a + b - 2.0 * a * b); }
    double XNOR(double a, double b) { return NOT(XOR(a, b)); }
    
    double MUX(double sel, double a, double b) {
        return OR(AND(sel, a), AND(NOT(sel), b));
    }
};

class GFNCompoundGates {
private:
    GFNGate* gate;
    
public:
    GFNCompoundGates(GFNGate* g) : gate(g) {}
    
    double NAND3(double a, double b, double c) { return gate->NAND(gate->NAND(a, b), c); }
    
    double MAJ3(double a, double b, double c) {
        return gate->OR(gate->OR(gate->AND(a, b), gate->AND(b, c)), gate->AND(a, c));
    }
    
    struct FullAdderResult { double sum; double carry; };
    
    FullAdderResult FULL_ADDER(double a, double b, double carry_in) {
        FullAdderResult r;
        r.sum = gate->XOR(gate->XOR(a, b), carry_in);
        r.carry = MAJ3(a, b, carry_in);
        return r;
    }
    
    FullAdderResult HALF_ADDER(double a, double b) { return FULL_ADDER(a, b, 0.0); }
    
    std::vector<double> RIPPLE_ADDER(const std::vector<double>& a, const std::vector<double>& b) {
        size_t n = std::min(a.size(), b.size());
        std::vector<double> sum(n);
        double carry = 0.0;
        for (size_t i = 0; i < n; i++) {
            auto fa = FULL_ADDER(a[i], b[i], carry);
            sum[i] = fa.sum;
            carry = fa.carry;
        }
        return sum;
    }
};

}
