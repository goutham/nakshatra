#ifndef __LAYER_H__
#define __LAYER_H__

#include "module.h"
#include "neuron.h"
#include "variable.h"

#include <iostream>
#include <vector>
#include <cstdint>

template <typename ValueType>
struct Layer : public Module<ValueType> {

    Layer(int nin, int nout, bool nonlin, std::optional<std::vector<double>::iterator>& w_iter) {
        for (int i = 0; i < nout; ++i) {
            neurons_.push_back(Neuron<ValueType>(nin, nonlin, w_iter));
        }
    }

    std::vector<ValueType> operator()(std::vector<uint64_t> x) const {
        std::vector<ValueType> out;
        for (size_t i = 0; i < neurons_.size(); ++i) {
            auto y = neurons_[i](x);
            // std::cout << "neuron out: " << y << std::endl;
            out.push_back(y);
        }
        return out;
    }

    std::vector<ValueType> operator()(std::vector<double> x) const {
        std::vector<ValueType> out;
        for (size_t i = 0; i < neurons_.size(); ++i) {
            auto y = neurons_[i](x);
            // std::cout << "neuron out: " << y << std::endl;
            out.push_back(y);
        }
        return out;
    }

    std::vector<ValueType> operator()(std::vector<Variable> x) const {
        std::vector<Variable> out;
        for (size_t i = 0; i < neurons_.size(); ++i) {
            auto y = neurons_[i](x);
            // std::cout << "neuron out: " << y << std::endl;
            out.push_back(y);
        }
        return out;
    }

    std::vector<ValueType> Parameters() const override {
        std::vector<ValueType> params;
        for (size_t i = 0; i < neurons_.size(); ++i) {
            auto nparams = neurons_[i].Parameters();
            params.insert(params.end(), nparams.begin(), nparams.end());
        }
        return params;
    }

    friend std::ostream& operator<<(std::ostream& os, const Layer& layer) {
        const auto params = layer.Parameters();
        for (size_t i = 0; i < params.size(); ++i) {
            os << "layer param: " << params[i] << std::endl;
        }
        return os;
    }

    std::vector<Neuron<ValueType>> neurons_;
};

#endif