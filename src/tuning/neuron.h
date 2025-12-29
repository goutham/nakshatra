#ifndef __NEURON_H__
#define __NEURON_H__

#include "module.h"
#include "variable.h"
#include "common.h"

#include <optional>
#include <random>
#include <cstdint>
#include <cmath>

template <typename ValueType>
ValueType Tanh(ValueType v);

template <>
inline Variable Tanh(Variable v) {
    return v.Tanh();
}

template <>
inline double Tanh(double v) {
    return tanh(v);
}

template <typename ValueType>
struct Neuron : public Module<ValueType> {
    Neuron(int nin, bool nonlin, std::optional<std::vector<double>::iterator>& w_iter) : nonlin_(nonlin) {
        if (!w_iter) {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_real_distribution<> dis(-1.0, 1.0);
            for (int i = 0; i < nin; ++i) {
                w_.push_back(ValueType(dis(gen)));
            }
            b_ = ValueType(dis(gen));
        } else {
            for (int i = 0; i < nin; ++i) {
                w_.push_back(ValueType(**w_iter));
                (*w_iter)++;
            }
            b_ = ValueType(**w_iter);
            (*w_iter)++;
        }
    }

    ValueType operator()(std::vector<uint64_t> bbs) const {
        ValueType activation = 0.0;
        for (size_t i = 0; i < bbs.size(); ++i) {
            uint64_t bb = bbs.at(i);
            while (bb) {
                const int index = Lsb1(bb);
                const int w_index = i * 64 + index;
                activation = activation + w_[w_index];
                bb ^= (1ULL << index);
            }
        }
        activation = activation + b_;
        return nonlin_ ? Tanh(activation) : activation;
    }

    ValueType operator()(std::vector<double> x) const {
        ValueType activation = 0.0;
        for (size_t i = 0; i < x.size(); ++i) {
            activation = activation + (w_[i] * x[i]);
        }
        activation = activation + b_;
        return nonlin_ ? Tanh(activation) : activation;
    }

    ValueType operator()(std::vector<Variable> x) const {
        ValueType activation = 0.0;
        for (size_t i = 0; i < x.size(); ++i) {
            activation = activation + (w_[i] * x[i]);
            // std::cout << "w_[i]: " << w_[i] << ", x[i]: " << x[i]
            //           << ", act: " << activation << std::endl;
        }
        activation = activation + b_;
        return nonlin_ ? Tanh(activation) : activation;
    }

    std::vector<ValueType> Parameters() const override {
        std::vector<ValueType> params;
        for (size_t i = 0; i < w_.size(); ++i) {
            params.push_back(w_[i]);
        }
        params.push_back(b_);
        return params;
    }

    friend std::ostream& operator<<(std::ostream& os, const Neuron& neuron) {
        const auto params = neuron.Parameters();
        for (size_t i = 0; i < params.size(); ++i) {
            os << "neuron param: " << params[i] << std::endl;
        }
        return os;
    }

    std::vector<ValueType> w_;
    ValueType b_ = 0.0;
    bool nonlin_ = false;
};

#endif