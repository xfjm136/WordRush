#pragma once

#include <filesystem>

namespace wordrush {

struct AppConfig {
  std::filesystem::path project_root;
  bool list_books = false;
};

int RunApplication(const AppConfig& config);

}  // namespace wordrush
