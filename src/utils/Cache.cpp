#include "Cache.h"

namespace accounting {

// 显式实例化常用类型
template class Cache<std::string, std::string>;
template class Cache<std::string, std::vector<std::string>>;

} // namespace accounting