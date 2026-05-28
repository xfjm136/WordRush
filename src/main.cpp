#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include "wordrush/application.hpp"

namespace {

std::filesystem::path DefaultRoot() {
  return std::filesystem::path(WORDRUSH_SOURCE_DIR);
}

void PrintHelp() {
  std::cout << "wordrush [dashboard|books] [--root /data/WordRush]\n";
}

}  // namespace

int main(int argc, char** argv) {
  wordrush::AppConfig config{
      .project_root = DefaultRoot(),
      .list_books = false,
  };

  std::vector<std::string> positional;
  for (int index = 1; index < argc; ++index) {
    const std::string arg = argv[index];
    if (arg == "--root") {
      if (index + 1 >= argc) {
        std::cerr << "--root expects a path\n";
        return 1;
      }
      config.project_root = argv[++index];
      continue;
    }
    if (arg == "--help" || arg == "-h") {
      PrintHelp();
      return 0;
    }
    positional.push_back(arg);
  }

  if (!positional.empty()) {
    if (positional.front() == "books") {
      config.list_books = true;
    } else if (positional.front() == "dashboard") {
      config.list_books = false;
    } else {
      PrintHelp();
      return 1;
    }
  }

  return wordrush::RunApplication(config);
}

