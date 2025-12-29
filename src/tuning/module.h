#ifndef __MODULE_H__
#define __MODULE_H__

#include "variable.h"
#include <vector>

template <typename ValueType>
struct Module {
  public:
    virtual void ZeroGrad() {
        if constexpr (std::is_same_v<ValueType, Variable>) {
            for (auto param : Parameters()) {
                param.data_->grad = 0.0;
            }
        }
    }

    virtual std::vector<ValueType> Parameters() const { return {}; }
};

#endif