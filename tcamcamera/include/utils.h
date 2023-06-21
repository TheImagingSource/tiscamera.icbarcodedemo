#pragma once
#include <memory>

///////////////////////////////////////////////////////
// Linux C++ 11 does not contain  std::make_unique

template<typename T, typename... Args>
std::unique_ptr<T> make_unique(Args&&... args) {
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}