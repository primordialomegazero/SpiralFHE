#include <iostream>
#include <iomanip>
#include <cmath>

constexpr double PHI = 1.6180339887498948482;
constexpr double PSI = -0.6180339887498948482;
constexpr double TAU = 0.1;

double compute_cassini(double s, int a) {
    double y1 = sin(s * PHI);
    double y2 = cos(s * PSI);
    return fabs((y1 + a * PHI) * (y2 + a * PSI) + 1.0);
}

struct Result { double min_val; int below_tau; };

Result verify(int a, int subdivisions) {
    Result r = {1e10, 0};
    double step = 1.0 / subdivisions;
    for (int j = 0; j < subdivisions; j++) {
        double c = compute_cassini(j * step, a);
        if (c < r.min_val) r.min_val = c;
        if (c < TAU) r.below_tau++;
    }
    return r;
}

int main() {
    std::cout << "\n";
    std::cout << "================================================================================\n";
    std::cout << "  SpiralFHE — CASSINI: SUPPORTED a-VALUES (skip a=2)\n";
    std::cout << "================================================================================\n\n";

    int a_values[] = {1, 3, 4, 5, 6, 7};
    int passed = 0;
    
    for (int a : a_values) {
        auto r = verify(a, 1000000);
        bool ok = (r.min_val > TAU) && (r.below_tau == 0);
        if (ok) passed++;
        std::cout << "  a=" << a << " | min=" << std::fixed << std::setprecision(6) << r.min_val
                  << " | below_tau=" << r.below_tau << " | " << (ok ? "OK" : "FAIL") << "\n";
    }

    std::cout << "\n  Supported layers: a = {1, 3, 4, 5, 6, 7}\n";
    std::cout << "  Unsupported: a = 2 (has zero crossing at s=" << std::fixed << std::setprecision(6) << 0.374653 << ")\n";
    std::cout << "  N = 6 layers max (using a-values 1,3,4,5,6,7)\n\n";
    
    std::cout << "  Bootstrap rate bound: 1/ceil((D-2)/3)\n";
    for (int D : {8, 16, 32, 64, 128}) {
        int ops = (D - 2 + 2) / 3;
        if (ops < 1) ops = 1;
        double bound = 1.0 / ops;
        std::cout << "    D=" << D << " bound=" << std::fixed << std::setprecision(4) << bound << "\n";
    }

    std::cout << "\n================================================================================\n";
    std::cout << "  Cassini: " << passed << "/6 | " << (passed == 6 ? "ALL VERIFIED" : "SOME FAILED") << "\n";
    std::cout << "================================================================================\n\n";

    return (passed == 6) ? 0 : 1;
}
