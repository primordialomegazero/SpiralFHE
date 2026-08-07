#pragma once
#include <cmath>
#include <vector>
#include "../core/constants.h"

// ═══════════════════════════════════════════════════════════════
// AUTO BOOTSTRAP CONTROLLER v5 — Lightweight, No Dependencies
// ═══════════════════════════════════════════════════════════════
// Standalone. No StabilityGuard. No SystemConfig. Just works.
// Fibonacci bridge for batch control. Φ-integrated state machine.

struct AutoBootstrap {
    // Configurable thresholds (all runtime-adjustable)
    double noise_watch     = 0.45;
    double noise_critical  = 0.85;
    double stab_watch      = 0.50;
    double stab_critical   = 0.15;
    double lyap_watch      = 0.15;
    double lyap_critical   = 0.02;
    int gates_until_action = 5;
    int gates_force_refresh = 50;
    int gates_batch_check  = 8;
    double batch_reduce_trigger = 0.60;
    double batch_increase_trigger = 0.40;
    int batch_min = 5;
    int batch_max = 500;
    
    // State
    enum CausalState { STATE_IDLE, STATE_WATCH, STATE_PREPARE, STATE_REFRESH, STATE_EMERGENCY };
    CausalState current_state = STATE_IDLE;
    CausalState previous_state = STATE_IDLE;
    
    // Tracking
    int gates_since_refresh = 0;
    int total_gates = 0;
    int total_refreshes = 0;
    int total_emergencies = 0;
    // ═══════════════════════════════════════════════════════════
    // Fractal Golden iO Integration
    // ═══════════════════════════════════════════════════════════
    int fractal_io_depth = 3;
    int N_obfuscation_rounds = 5;
    bool iO_enabled = true;
    int iO_mode = 0;  // 0=STRUCTURAL_IO, 1=BLACKHOLE, 2=OFF
    int recommended_batch = 32;
    int fib_index = 7;
    double current_noise = 0, current_stability = 0, current_lyapunov = 0;
    
    // Fibonacci
    int fib(int n) { if (n <= 0) return 1; if (n == 1) return 1; int a=1,b=1; for(int i=2;i<=n;i++){int c=a+b;a=b;b=c;} return b; }
    int clamp_fib(int n) { int f=fib(n); if(f<batch_min)return batch_min; if(f>batch_max)return batch_max; return f; }
    void set_batch(int b) { recommended_batch=std::max(batch_min,std::min(batch_max,b)); fib_index=3; while(fib(fib_index)<recommended_batch&&fib_index<15)fib_index++; recommended_batch=clamp_fib(fib_index); }
    
    // Φ computation
    double get_phi() const { return (current_noise + (1.0-current_stability) + (1.0-current_lyapunov))/3.0; }
    
    // Sense
    CausalState sense(double noise, double stability, double lyapunov) {
        gates_since_refresh++; total_gates++;
        current_noise=noise; current_stability=stability; current_lyapunov=lyapunov;
        previous_state=current_state;
        
        // Force refresh
        if (gates_since_refresh > gates_force_refresh) { current_state=STATE_REFRESH; total_refreshes++; apply_state(STATE_REFRESH); return STATE_REFRESH; }
        // Emergency
        if (noise>noise_critical||stability<stab_critical||lyapunov<lyap_critical) { current_state=STATE_EMERGENCY; total_emergencies++; apply_state(STATE_EMERGENCY); return STATE_EMERGENCY; }
        // Progression
        if (gates_since_refresh > gates_until_action) {
            if (noise>0.75||stability<0.25||lyapunov<0.04) { current_state=STATE_REFRESH; total_refreshes++; apply_state(STATE_REFRESH); return STATE_REFRESH; }
            if (noise>0.60||stability<0.35||lyapunov<0.06) { current_state=STATE_PREPARE; apply_state(STATE_PREPARE); return STATE_PREPARE; }
            if (noise>noise_watch||stability<stab_watch||lyapunov<lyap_watch) { current_state=STATE_WATCH; apply_state(STATE_WATCH); return STATE_WATCH; }
        }
        current_state=STATE_IDLE; apply_state(STATE_IDLE); return STATE_IDLE;
    }
    
    // Apply state
    void apply_state(CausalState s) {
        switch(s) {
            case STATE_EMERGENCY: fib_index=3; recommended_batch=batch_min; gates_since_refresh=0; break;
            case STATE_REFRESH: gates_since_refresh=0; if(current_noise>batch_reduce_trigger){fib_index=std::max(3,fib_index-2);recommended_batch=clamp_fib(fib_index);} break;
            case STATE_PREPARE: if(current_noise>0.55){fib_index=std::max(3,fib_index-1);recommended_batch=clamp_fib(fib_index);} break;
            case STATE_WATCH: if(current_noise>noise_watch&&gates_since_refresh>gates_batch_check){fib_index=std::max(3,fib_index-1);recommended_batch=clamp_fib(fib_index);} break;
            case STATE_IDLE: if(current_noise<batch_increase_trigger&&gates_since_refresh>gates_batch_check*2){fib_index=std::min(15,fib_index+2);recommended_batch=clamp_fib(fib_index);} break;
        }
    }
    
    // Queries
    bool should_bootstrap() const { return current_state==STATE_REFRESH||current_state==STATE_EMERGENCY; }
    void reset() { gates_since_refresh=0; current_state=STATE_IDLE; previous_state=STATE_IDLE; }
    const char* status() const { switch(current_state){case STATE_IDLE:return"IDLE";case STATE_WATCH:return"WATCH";case STATE_PREPARE:return"PREPARE";case STATE_REFRESH:return"REFRESH";case STATE_EMERGENCY:return"EMERGENCY";default:return"UNKNOWN";} }
    bool state_changed() const { return current_state!=previous_state; }
};
