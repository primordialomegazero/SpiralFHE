#pragma once
#include <vector>
#include <string>
#include <cmath>

namespace SpiralFHE {

enum class GateType { NAND, AND, OR, NOT, XOR, XNOR, MUX };

struct Gate {
    GateType type;
    int input_a;
    int input_b;
    int output;
};

struct CompiledCircuit {
    std::vector<Gate> gates;
    int num_inputs;
    int num_outputs;
    int num_wires;
};

class UniversalCompiler {
public:
    static CompiledCircuit from_truth_table(const std::vector<int>& truth_table, int num_inputs) {
        CompiledCircuit cc;
        cc.num_inputs = num_inputs;
        cc.num_outputs = 1;
        cc.num_wires = num_inputs;
        
        int num_entries = 1 << num_inputs;
        std::vector<int> table = truth_table;
        table.resize(num_entries, 0);
        
        std::vector<int> product_terms;
        for (int i = 0; i < num_entries; i++) {
            if (table[i] == 0) continue;
            
            int first_literal = -1;
            for (int j = 0; j < num_inputs; j++) {
                int bit = (i >> j) & 1;
                int literal_wire = j;
                
                if (bit == 0) {
                    Gate not_gate;
                    not_gate.type = GateType::NOT;
                    not_gate.input_a = j;
                    not_gate.input_b = -1;
                    not_gate.output = cc.num_wires++;
                    cc.gates.push_back(not_gate);
                    literal_wire = not_gate.output;
                }
                
                if (first_literal < 0) {
                    first_literal = literal_wire;
                } else {
                    Gate and_gate;
                    and_gate.type = GateType::AND;
                    and_gate.input_a = first_literal;
                    and_gate.input_b = literal_wire;
                    and_gate.output = cc.num_wires++;
                    cc.gates.push_back(and_gate);
                    first_literal = and_gate.output;
                }
            }
            
            if (first_literal >= 0) product_terms.push_back(first_literal);
        }
        
        if (product_terms.empty()) {
            cc.num_wires = num_inputs + 1;
            cc.gates.clear();
            cc.num_outputs = num_inputs;
        } else if (product_terms.size() == 1) {
            cc.num_outputs = product_terms[0];
        } else {
            int or_wire = product_terms[0];
            for (size_t i = 1; i < product_terms.size(); i++) {
                Gate or_gate;
                or_gate.type = GateType::OR;
                or_gate.input_a = or_wire;
                or_gate.input_b = product_terms[i];
                or_gate.output = cc.num_wires++;
                cc.gates.push_back(or_gate);
                or_wire = or_gate.output;
            }
            cc.num_outputs = or_wire;
        }
        
        return cc;
    }
    
    static CompiledCircuit half_adder() {
        CompiledCircuit cc;
        cc.num_inputs = 2;
        cc.num_outputs = 3;
        cc.num_wires = 4;
        
        Gate xor_gate = {GateType::XOR, 0, 1, 2};
        Gate and_gate = {GateType::AND, 0, 1, 3};
        cc.gates.push_back(xor_gate);
        cc.gates.push_back(and_gate);
        
        return cc;
    }
    
    static CompiledCircuit full_adder() {
        CompiledCircuit cc;
        cc.num_inputs = 3;
        cc.num_outputs = 9;
        cc.num_wires = 10;
        
        cc.gates.push_back({GateType::XOR, 0, 1, 3});
        cc.gates.push_back({GateType::XOR, 3, 2, 6});
        cc.gates.push_back({GateType::AND, 0, 1, 4});
        cc.gates.push_back({GateType::AND, 1, 2, 5});
        cc.gates.push_back({GateType::OR,  4, 5, 7});
        cc.gates.push_back({GateType::AND, 0, 2, 8});
        cc.gates.push_back({GateType::OR,  7, 8, 9});
        
        return cc;
    }
};

class GFNGateEvaluator {
public:
    static std::vector<double> evaluate(const CompiledCircuit& cc, const std::vector<double>& inputs) {
        std::vector<double> wires(cc.num_wires, 0.0);
        
        for (size_t i = 0; i < inputs.size() && i < (size_t)cc.num_inputs; i++) {
            wires[i] = inputs[i];
        }
        
        for (const auto& gate : cc.gates) {
            double a = (gate.input_a >= 0) ? wires[gate.input_a] : 0.0;
            double b = (gate.input_b >= 0) ? wires[gate.input_b] : 0.0;
            
            switch (gate.type) {
                case GateType::NAND: wires[gate.output] = fabs(1.0 - a * b); break;
                case GateType::AND:  wires[gate.output] = fabs(a * b); break;
                case GateType::OR:   wires[gate.output] = fabs(a + b - a * b); break;
                case GateType::NOT:  wires[gate.output] = fabs(1.0 - a); break;
                case GateType::XOR:  wires[gate.output] = fabs(a + b - 2.0 * a * b); break;
                case GateType::XNOR: wires[gate.output] = fabs(1.0 - fabs(a + b - 2.0 * a * b)); break;
                case GateType::MUX:  wires[gate.output] = fabs(fabs(a * 1.0) + fabs((1.0 - a) * b) - fabs(a * 1.0) * fabs((1.0 - a) * b)); break;
            }
        }
        
        return wires;
    }
    
    static double GFN_NAND(double a, double b) { return fabs(1.0 - a * b); }
    static double GFN_AND(double a, double b)  { return fabs(a * b); }
    static double GFN_OR(double a, double b)   { return fabs(a + b - a * b); }
    static double GFN_NOT(double a)            { return fabs(1.0 - a); }
    static double GFN_XOR(double a, double b)  { return fabs(a + b - 2.0 * a * b); }
    static double GFN_XNOR(double a, double b) { return GFN_NOT(GFN_XOR(a, b)); }
    static double GFN_MUX(double sel, double a, double b) { 
        return GFN_OR(GFN_AND(sel, a), GFN_AND(GFN_NOT(sel), b)); 
    }
    
    static bool to_bool(double v) { return fabs(v) >= 0.5; }
};

}
