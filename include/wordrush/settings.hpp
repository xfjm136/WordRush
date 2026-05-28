#pragma once

#include <cstddef>
#include <filesystem>

namespace wordrush {

struct UiSettings {
  std::size_t theme_index = 0;
  bool use_theme_background = true;
};

UiSettings LoadUiSettings(const std::filesystem::path& project_root);
bool SaveUiSettings(const std::filesystem::path& project_root, const UiSettings& settings);

}  // namespace wordrush

