#pragma once

#include <string>
#include <vector>

#include <ftxui/screen/color.hpp>

namespace wordrush {

struct ThemePalette {
  ftxui::Color background;
  ftxui::Color surface;
  ftxui::Color surface_alt;
  ftxui::Color focus;
  ftxui::Color text;
  ftxui::Color muted;
  ftxui::Color subtle;
  ftxui::Color blue;
  ftxui::Color green;
  ftxui::Color peach;
  ftxui::Color info;
  ftxui::Color success;
  ftxui::Color warning;
  ftxui::Color accent;
  ftxui::Color overlay;
  bool use_background = true;
  std::string name;
};

std::vector<ThemePalette> BuiltinThemes();
ThemePalette ThemeByIndex(std::size_t index, bool use_background);

}  // namespace wordrush
