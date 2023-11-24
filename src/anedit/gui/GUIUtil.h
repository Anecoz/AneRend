#pragma once

#include <algorithm>
#include <string>
#include <vector>

namespace gui {

template <typename T>
void filterName(std::vector<T>& v, std::string filter)
{
  v.erase(std::remove_if(
    v.begin(), v.end(),
    [&filter](T& t) {
      return t._name.find(filter) != std::string::npos;
    }
  ), v.end());
}

}