#ifndef __MLP_H__
#define __MLP_H__

#include "layer.h"
#include "module.h"
#include "variable.h"

#include <optional>
#include <vector>
#include <cstdint>
#include <iostream>

template <typename ValueType>
struct MLP : public Module<ValueType> {

    MLP(int nin, std::vector<int> nouts, std::vector<double> weights = {}): nin_(nin), nouts_(nouts) {
        std::vector<int> sz;
        sz.push_back(nin);
        for (size_t i = 0; i < nouts.size(); ++i) {
            sz.push_back(nouts[i]);
        }
        std::optional<std::vector<double>::iterator> w_iter;
        if (!weights.empty()) {
            w_iter = weights.begin();
        }
        for (size_t i = 0; i < nouts.size(); ++i) {
            layers_.push_back(Layer<ValueType>(sz[i], sz[i + 1], i != nouts.size() - 1, w_iter));
        }
        if (!weights.empty()) {
            assert(w_iter == weights.end());
        }
    }

    std::vector<ValueType> operator()(std::vector<uint64_t> x) const {
        assert(layers_.size() > 0);
        auto y = layers_.at(0)(x);
        for (size_t i = 1; i < layers_.size(); ++i) {
            y = layers_.at(i)(y);
        }
        return y;
    }

    std::vector<ValueType> operator()(std::vector<double> x) const {
        assert(layers_.size() > 0);
        auto y = layers_.at(0)(x);
        for (size_t i = 1; i < layers_.size(); ++i) {
            y = layers_.at(i)(y);
        }
        return y;
    }

    std::vector<ValueType> operator()(std::vector<Variable> x) const {
        for (auto& layer : layers_) {
            x = layer(x);
            // for (int i = 0; i < x.size(); ++i) {
            //     std::cout << "layer out: " << x[i] << std::endl;
            // }
        }
        return x;
    }

    std::vector<ValueType> Parameters() const override {
        std::vector<ValueType> params;
        for (auto& layer : layers_) {
            auto p = layer.Parameters();
            params.insert(params.end(), p.begin(), p.end());
        }
        return params;
    }

    const int nin_;
    const std::vector<int> nouts_;
    std::vector<Layer<ValueType>> layers_;
};

#endif