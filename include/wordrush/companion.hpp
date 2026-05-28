#pragma once

#include <string>
#include <vector>

#include "wordrush/types.hpp"

namespace wordrush {

struct CompanionState {
  std::string name;
  std::string mood;
  std::string posture;
  std::string ascii_art;
  std::string speech;
  std::vector<std::string> actions;
  int affinity = 0;
  int level = 1;
};

CompanionState BuildCompanionState(const std::vector<WordBookOverview>& books);
CompanionState BuildCompanionState(const WordBookDetail& book, int correct, int wrong, std::size_t index);

}  // namespace wordrush

