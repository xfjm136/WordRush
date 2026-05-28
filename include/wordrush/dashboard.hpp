#pragma once

#include <filesystem>
#include <vector>

#include "wordrush/storage.hpp"
#include "wordrush/training.hpp"
#include "wordrush/types.hpp"

namespace wordrush {

class Dashboard {
 public:
  Dashboard(std::vector<WordBookOverview> books, std::filesystem::path project_root);

  int Run();

 private:
  std::vector<WordBookOverview> books_;
  std::filesystem::path project_root_;
  TrainingPlanner planner_;
};

}  // namespace wordrush
