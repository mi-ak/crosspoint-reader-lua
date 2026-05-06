#pragma once
#include <cstdint>
#include <string>
#include "HalStorage.h"  // for FsFile

namespace serialization {

template <typename T>
inline void readPod(FsFile&, T& value) {
  value = T{};
}

inline void readString(FsFile&, std::string& value) {
  value.clear();
}

}  // namespace serialization
