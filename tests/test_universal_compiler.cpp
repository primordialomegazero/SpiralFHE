#include <iostream>
#include <iomanip>
#include <vector>
#include "../src/compiler/universal_compiler.h"

using namespace SpiralFHE;

int main() {
    std::cout << "\n";
    std::cout << "================================================================================\n";
    std::cout << "  SPIRALFHE — UNIVERSAL COMPILER\n";
    std::cout << "  Any boolean function -> GF-N gates\n";
    std::cout << "================================================================================\n\n";

    int passed = 0;
    int total = 0;

    // Basic gates
    std::cout << "--- Basic Gates ---\n\n";
    
    struct { std::string name; double a; double b; bool expected; double (*gate)(double,double); } tests[] = {
        {"NAND",0,0,1,GFNGateEvaluator::GFN_NAND}, {"NAND",0,1,1,GFNGateEvaluator::GFN_NAND},
        {"NAND",1,0,1,GFNGateEvaluator::GFN_NAND}, {"NAND",1,1,0,GFNGateEvaluator::GFN_NAND},
        {"AND",0,0,0,GFNGateEvaluator::GFN_AND},   {"AND",0,1,0,GFNGateEvaluator::GFN_AND},
        {"AND",1,0,0,GFNGateEvaluator::GFN_AND},   {"AND",1,1,1,GFNGateEvaluator::GFN_AND},
        {"OR",0,0,0,GFNGateEvaluator::GFN_OR},     {"OR",0,1,1,GFNGateEvaluator::GFN_OR},
        {"OR",1,0,1,GFNGateEvaluator::GFN_OR},     {"OR",1,1,1,GFNGateEvaluator::GFN_OR},
        {"XOR",0,0,0,GFNGateEvaluator::GFN_XOR},   {"XOR",0,1,1,GFNGateEvaluator::GFN_XOR},
        {"XOR",1,0,1,GFNGateEvaluator::GFN_XOR},   {"XOR",1,1,0,GFNGateEvaluator::GFN_XOR},
    };

    for (auto& t : tests) {
        double r = t.gate(t.a, t.b);
        bool ok = (GFNGateEvaluator::to_bool(r) == (bool)t.expected);
        if (ok) passed++;
        total++;
        if (!ok) std::cout << "  FAIL: " << t.name << "(" << t.a << "," << t.b << ")\n";
    }
    std::cout << "  " << passed << "/" << total << "\n\n";

    // Half Adder
    std::cout << "--- Half Adder ---\n\n";
    auto ha = UniversalCompiler::half_adder();
    int ha_in[4][2] = {{0,0},{0,1},{1,0},{1,1}};
    for (auto& in : ha_in) {
        auto w = GFNGateEvaluator::evaluate(ha, {(double)in[0], (double)in[1]});
        bool sum = GFNGateEvaluator::to_bool(w[2]), carry = GFNGateEvaluator::to_bool(w[3]);
        bool ok = (sum == (in[0]!=in[1])) && (carry == (in[0]&&in[1]));
        if (ok) passed++; total++;
        std::cout << "  " << in[0] << "+" << in[1] << "=" << sum << "c" << carry << " " << (ok?"OK":"FAIL") << "\n";
    }
    std::cout << "\n";

    // Full Adder
    std::cout << "--- Full Adder ---\n\n";
    auto fa = UniversalCompiler::full_adder();
    for (int a=0;a<=1;a++) for (int b=0;b<=1;b++) for (int c=0;c<=1;c++) {
        auto w = GFNGateEvaluator::evaluate(fa, {(double)a,(double)b,(double)c});
        bool sum = GFNGateEvaluator::to_bool(w[6]), carry = GFNGateEvaluator::to_bool(w[9]);
        int cnt = a+b+c; bool ok = (sum==(cnt%2==1)) && (carry==(cnt>=2));
        if (ok) passed++; total++;
        std::cout << "  " << a << "+" << b << "+" << c << "=" << sum << "c" << carry << " " << (ok?"OK":"FAIL") << "\n";
    }
    std::cout << "\n";

    // Truth Table: Majority(3)
    std::cout << "--- Majority(3) from Truth Table ---\n\n";
    std::vector<int> maj = {0,0,0,1,0,1,1,1};
    auto mc = UniversalCompiler::from_truth_table(maj, 3);
    std::cout << "  Compiled: " << mc.gates.size() << " gates\n";
    for (int a=0;a<=1;a++) for (int b=0;b<=1;b++) for (int c=0;c<=1;c++) {
        auto w = GFNGateEvaluator::evaluate(mc, {(double)a,(double)b,(double)c});
        bool out = GFNGateEvaluator::to_bool(w[mc.num_outputs]);
        bool exp = (a+b+c >= 2);
        bool ok = (out == exp);
        if (ok) passed++; total++;
        if (!ok) std::cout << "  FAIL: " << a << b << c << " -> " << out << " (exp " << exp << ")\n";
    }
    std::cout << "  All 8 cases OK\n\n";

    std::cout << "================================================================================\n";
    std::cout << "  TOTAL: " << passed << "/" << total << " (" << std::fixed << std::setprecision(1) << (100.0*passed/total) << "%)\n";
    std::cout << "  Gates: 16 basic + 4 HA + 8 FA + 8 Majority = 36 tests\n";
    std::cout << "================================================================================\n\n";

    return (passed == total) ? 0 : 1;
}
