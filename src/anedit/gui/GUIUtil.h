#pragma once

#include "../logic/AneditContext.h"

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
 
// TODO: Implement this and manage multiselect, probably via AneditContext.
void updateSelection(util::Uuid& id, logic::AneditContext::SelectionType type, logic::AneditContext* c)
{
  
}

}