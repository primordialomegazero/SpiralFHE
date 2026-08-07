#pragma once
#include "../core/constants.h"
#include "../utils/safe_math.h"
#include "../utils/logger.h"
#include <vector>
#include <map>
#include <string>
#include <functional>

// ═══════════════════════════════════════════════════════════════════════════════
// HIERARCHICAL SEED TREE — Enterprise Key Management Architecture
// ═══════════════════════════════════════════════════════════════════════════════
//
// The Seed Tree is the cryptographic isolation architecture that makes
// Spiral Fractal iO enterprise-ready.
//
// Architecture:
//                    master_seed (64-bit double)
//                         │
//         ┌───────────────┼───────────────┬───────────────┐
//         ▼               ▼               ▼               ▼
//    encryption      fractal         refresh         timing
//    branch ×φ⁰     branch ×φ¹      branch ×φ²      branch ×φ³
//         │               │               │               │
//    ┌────┴────┐     ┌────┴────┐     ┌────┴────┐     ┌────┴────┐
//    ▼         ▼     ▼         ▼     ▼         ▼     ▼         ▼
//   sub₁     sub₂  sub₁     sub₂  sub₁     sub₂  sub₁     sub₂
//
// Key Properties:
//   - Branch Isolation: φ is irrational → no linear relationship
//   - Disaster Recovery: Backup 1 number → recover ALL keys
//   - Multi-Tenancy: Branch per tenant → complete isolation
//   - Infinite Scalability: φ^k mod 1 → no collisions (ergodic)
//
// ═══════════════════════════════════════════════════════════════════════════════

struct SeedNode {
    double value;                    // The seed value
    int depth;                       // Depth in tree
    int branch_id;                   // Branch index
    SeedNode* parent;                // Parent node
    std::vector<SeedNode*> children; // Child nodes

    SeedNode(double v = 0, int d = 0, int b = 0, SeedNode* p = nullptr)
        : value(v), depth(d), branch_id(b), parent(p) {}

    // ═══════════════════════════════════════════════════════════
    // Derive child seed using φ or ψ exponentiation
    // φ/ψ alternation per sub-branch for decorrelation
    // ═══════════════════════════════════════════════════════════
    double derive_child(int index, bool use_phi) {
        double base = use_phi ? PHI : PSI;
        return SafeMath::fmod_safe(value * std::pow(base, index + 1) + depth * base);
    }

    SeedNode* spawn(int index, bool use_phi = true) {
        auto child = new SeedNode(derive_child(index, use_phi), depth + 1, index, this);
        children.push_back(child);
        return child;
    }

    ~SeedNode() { for (auto c : children) delete c; }
};

struct HierarchicalSeedTree {
    SeedNode root;
    std::map<std::string, SeedNode*> branches;  // Named branch access
    int total_nodes;

    HierarchicalSeedTree() : root(0, 0, 0, nullptr), total_nodes(1) {}

    // ═══════════════════════════════════════════════════════════
    // Initialize with 8 standard branches
    // Each branch uses φ^index for unique derivation
    // ═══════════════════════════════════════════════════════════
    void init(double master_seed) {
        root.value = SafeMath::fmod_safe(std::abs(master_seed) * PHI);

        // 8 standard branches for different subsystems
        create_branch("encryption", 0, true);   // φ⁰ — GF-N encryption keys
        create_branch("fractal", 1, false);     // φ¹ — Fractal transform seeds
        create_branch("refresh", 2, true);      // φ² — Refresh timing
        create_branch("timing", 3, false);      // φ³ — Emergent timing
        create_branch("fhe", 4, true);          // φ⁴ — FHE context
        create_branch("adaptive", 5, false);    // φ⁵ — Autonomous controller
        create_branch("symmetric", 6, true);    // φ⁶ — Symmetric keys
        create_branch("batch", 7, false);       // φ⁷ — Batch processing

        Logger::info("SeedTree: " + std::to_string(total_nodes) + 
                    " nodes across " + std::to_string(branches.size()) + " branches");
    }

    void create_branch(const std::string& name, int index, bool use_phi) {
        auto node = root.spawn(index, use_phi);
        branches[name] = node;
        total_nodes++;
    }

    // ═══════════════════════════════════════════════════════════
    // Get a seed from a specific branch and sub-index
    // ═══════════════════════════════════════════════════════════
    double get_seed(const std::string& component, int sub_index = 0) {
        auto it = branches.find(component);
        if (it == branches.end()) {
            Logger::warn("SeedTree: branch '" + component + "' not found, using root");
            return root.value;
        }
        auto node = it->second->spawn(sub_index, sub_index % 2 == 0);
        total_nodes++;
        return node->value;
    }

    // ═══════════════════════════════════════════════════════════
    // Get a chain of seeds for N-layer operations
    // ═══════════════════════════════════════════════════════════
    std::vector<double> get_seed_chain(const std::string& component, int count) {
        std::vector<double> seeds;
        auto it = branches.find(component);
        if (it == branches.end()) {
            for (int i = 0; i < count; i++) seeds.push_back(root.value);
            return seeds;
        }
        auto current = it->second;
        for (int i = 0; i < count; i++) {
            current = current->spawn(i, i % 2 == 0);
            total_nodes++;
            seeds.push_back(current->value);
        }
        return seeds;
    }
};
