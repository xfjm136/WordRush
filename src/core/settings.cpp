#include "wordrush/settings.hpp"

#include <fstream>
#include <string>

namespace wordrush {

namespace {

std::filesystem::path SettingsPath(const std::filesystem::path& project_root) {
  return project_root / "data" / "state" / "ui-settings.conf";
}

}  // namespace

UiSettings LoadUiSettings(const std::filesystem::path& project_root) {
  UiSettings settings;
  std::ifstream input(SettingsPath(project_root));
  if (!input) {
    return settings;
  }

  std::string line;
  while (std::getline(input, line)) {
    const auto pos = line.find('=');
    if (pos == std::string::npos) {
      continue;
    }
    const std::string key = line.substr(0, pos);
    const std::string value = line.substr(pos + 1);
    if (key == "theme_index") {
      settings.theme_index = static_cast<std::size_t>(std::stoul(value));
    } else if (key == "use_theme_background") {
      settings.use_theme_background = value == "1";
    }
  }

  return settings;
}

bool SaveUiSettings(const std::filesystem::path& project_root, const UiSettings& settings) {
  std::filesystem::create_directories((project_root / "data" / "state"));
  std::ofstream output(SettingsPath(project_root), std::ios::trunc);
  if (!output) {
    return false;
  }

  output << "theme_index=" << settings.theme_index << '\n';
  output << "use_theme_background=" << (settings.use_theme_background ? 1 : 0) << '\n';
  return static_cast<bool>(output);
}

}  // namespace wordrush

