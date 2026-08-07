#pragma once
#include <cmath>

struct SafeMath {
    static double fmod_safe(double x) {
        if (std::isnan(x) || std::isinf(x)) return 0.5;
        double r = std::fmod(std::abs(x), 1.0);
        return (std::isnan(r) || std::isinf(r)) ? 0.5 : r;
    }
    static double div_safe(double a, double b) {
        if (std::abs(b) < 0.0000001) b = (b >= 0 ? 0.0000001 : -0.0000001);
        double r = a / b;
        return (std::isnan(r) || std::isinf(r)) ? 0.0 : r;
    }
    static double sqrt_safe(double x) {
        if (std::isnan(x) || x < 0.0) return 0.0;
        double r = std::sqrt(x);
        return (std::isnan(r) || std::isinf(r)) ? 0.0 : r;
    }
    static double pow_safe(double base, double exp) {
        if (std::isnan(base) || std::isnan(exp)) return 0.5;
        if (base < 0.0 && std::abs(exp - std::round(exp)) > 0.0001) base = std::abs(base);
        double r = std::pow(std::abs(base), exp);
        return (std::isnan(r) || std::isinf(r)) ? 0.5 : r;
    }
    static double exp_safe(double x) {
        if (std::isnan(x)) return 1.0;
        x = std::max(-50.0, std::min(50.0, x));
        double r = std::exp(x);
        return (std::isnan(r) || std::isinf(r)) ? 1.0 : r;
    }
    static double sin_safe(double x) { return (std::isnan(x)||std::isinf(x)) ? 0.0 : std::sin(x); }
    static double cos_safe(double x) { return (std::isnan(x)||std::isinf(x)) ? 1.0 : std::cos(x); }
};
