#ifndef __VARIABLE_H__
#define __VARIABLE_H__

#include <cmath>
#include <functional>
#include <iostream>
#include <memory>
#include <unordered_set>
#include <vector>

struct Variable {

  struct Data {
    double value = 0.0;
    double grad = 0.0;
    double velocity = 0.0;
    std::vector<Variable> children;
    std::function<void()> backward = [] {};
  };

  struct Hash {
    size_t operator()(const Variable& value) const {
      return std::hash<std::shared_ptr<Data>>()(value.data_);
    }
  };

  Variable() { data_ = std::make_shared<Data>(); };

  Variable(double value, std::vector<Variable> children) {
    data_ = std::make_shared<Data>();
    data_->value = value;
    data_->children = children;
  }

  Variable(const Variable& other) { data_ = other.data_; }

  Variable(double other) {
    data_ = std::make_shared<Data>();
    data_->value = other;
  }

  Variable operator=(Variable other) {
    data_ = other.data_;
    return *this;
  }

  Variable operator=(double value) {
    data_->value = value;
    return *this;
  }

  Variable operator+(Variable other) const {
    Variable out(data_->value + other.data_->value, {*this, other});
    out.data_->backward = [self = *this, other,
                           out_data = std::weak_ptr<Data>(out.data_)]() {
      auto out_data_ptr = out_data.lock();
      self.data_->grad += out_data_ptr->grad;
      other.data_->grad += out_data_ptr->grad;
    };
    return out;
  }

  Variable operator+(double value) const { return *this + Variable(value); }

  Variable operator*(Variable other) const {
    Variable out(data_->value * other.data_->value, {*this, other});
    out.data_->backward = [self = *this, other,
                           out_data = std::weak_ptr<Data>(out.data_)]() {
      auto out_data_ptr = out_data.lock();
      self.data_->grad += other.data_->value * out_data_ptr->grad;
      other.data_->grad += self.data_->value * out_data_ptr->grad;
    };
    return out;
  }

  Variable operator*(double other) const { return *this * Variable(other, {}); }

  Variable operator+=(Variable other) const {
    data_ = (*this + other).data_;
    return *this;
  }

  Variable Power(double exponent) {
    Variable out(std::pow(data_->value, exponent), {*this});
    out.data_->backward = [self = *this,
                           out_data = std::weak_ptr<Data>(out.data_),
                           exponent]() {
      self.data_->grad += exponent * std::pow(self.data_->value, exponent - 1) *
                          out_data.lock()->grad;
    };
    return out;
  }

  Variable Relu() {
    Variable out(data_->value < 0 ? 0 : data_->value, {*this});
    out.data_->backward = [self = *this,
                           out_data = std::weak_ptr<Data>(out.data_)]() {
      auto out_data_ptr = out_data.lock();
      self.data_->grad += (out_data_ptr->value > 0) * out_data_ptr->grad;
    };
    return out;
  }

  Variable Tanh() {
    double t = (exp(2 * data_->value) - 1) / (exp(2 * data_->value) + 1);
    Variable out(t, {*this});
    out.data_->backward = [self = *this,
                           out_data = std::weak_ptr<Data>(out.data_), t]() {
      self.data_->grad += (1.0 - t * t) * out_data.lock()->grad;
    };
    return out;
  }

  Variable Sigmoid(double multiplier = 1.0) {
    double t = 1.0 / (1.0 + exp(-data_->value * multiplier));
    Variable out(t, {*this});
    out.data_->backward = [self = *this,
                           out_data = std::weak_ptr<Data>(out.data_), t]() {
      self.data_->grad += t * (1.0 - t) * out_data.lock()->grad;
    };
    return out;
  }

  Variable operator-() const { return *this * -1; }

  Variable operator-(Variable other) const { return *this + (-other); }

  Variable operator-(double value) const { return *this - Variable(value); }

  Variable operator/(Variable other) const { return *this * other.Power(-1); }

  bool operator==(const Variable& other) const {
    return this->data_ == other.data_;
  }

  friend std::ostream& operator<<(std::ostream& os, const Variable& val) {
    os << val.data_->value;
    // os << "value: " << val.data_->value << ", grad: " << val.data_->grad;
    return os;
  }

  friend Variable operator+(double a, Variable b) { return b + a; }

  friend Variable operator*(double a, Variable b) { return b * a; }

  explicit operator double() const { return data_->value; }

  void TopoSort(std::unordered_set<Variable, Hash>& visited,
                std::vector<Variable>& topo, Variable v) {
    if (visited.find(v) == visited.end()) {
      visited.insert(v);
      for (auto child : v.data_->children) {
        TopoSort(visited, topo, child);
      }
      topo.push_back(v);
    }
  }

  std::vector<Variable> TopoSort() {
    std::vector<Variable> topo;
    std::unordered_set<Variable, Hash> visited;
    TopoSort(visited, topo, *this);
    return topo;
  }

  size_t Size() const {
    size_t size = 1;
    for (size_t i = 0; i < data_->children.size(); ++i) {
      size += data_->children.at(i).Size();
    }
    return size;
  }

  void Backward() {
    auto topo = TopoSort();
    data_->grad = 1.0;
    // std::cout << "top.size: " << topo.size() << std::endl;
    for (int i = topo.size() - 1; i >= 0; --i) {
      // std::cout << "calling backward on " << topo[i] << std::endl;
      topo[i].data_->backward();
    }
  }

  mutable std::shared_ptr<Data> data_;
};

#endif