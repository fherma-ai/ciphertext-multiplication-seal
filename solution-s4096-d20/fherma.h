// GENERATED from ciphertext_multiplication/ckks@1.0.0. Do not edit — `--update` rewrites it.
//
// The types your answer is written against, derived from the signature: one
// field per value parameter, per argument, per result. A tensor is typed,
// because the signature already settled what its elements are.
#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace fherma {

template <class T>
struct Tensor {
    std::vector<int64_t> shape;
    std::vector<T> data;          // row-major

    int64_t count() const {
        int64_t total = 1;
        for (auto d : shape) total *= d;
        return total;
    }
};

struct Point {
    uint32_t slots = 0;   // u32
    uint32_t depth = 0;   // u32
    uint32_t security = 0;   // u32
    uint32_t precision = 0;   // u32
};

struct Inputs {
    Tensor<double> a;   // tensor<slots x f64>
    Tensor<double> b;   // tensor<slots x f64>
};

struct Outputs {
    Tensor<double> c;   // tensor<slots x f64>
};

}  // namespace fherma

// The four functions you write are declared in solve.h.
