#include "wordrush/application.hpp"

#include <iomanip>
#include <iostream>
#include <string>

#include "wordrush/dashboard.hpp"
#include "wordrush/storage.hpp"
#include "wordrush/drill_session.hpp"

namespace wordrush {

namespace {

void PrintBooks(const std::vector<WordBookOverview>& books) {
  std::cout << std::left << std::setw(20) << "词本" << std::setw(14) << "考试" << std::setw(8) << "总数"
            << std::setw(8) << "到期" << std::setw(10) << "掌握" << std::setw(10) << "正确率" << "分类"
            << '\n';

  for (const auto& book : books) {
    std::cout << std::left << std::setw(20) << book.meta.id << std::setw(14) << book.meta.exam << std::setw(8)
              << book.meta.total_entries << std::setw(8) << book.progress.due_now << std::setw(10)
              << book.progress.mastered_items << std::setw(10)
              << (std::to_string(static_cast<int>(book.progress.accuracy * 100.0)) + "%") << book.meta.track
              << '\n';
  }
}

}  // namespace

int RunApplication(const AppConfig& config) {
  WordBookRepository repository(config.project_root);
  auto books = repository.LoadOverview();

  if (config.list_books) {
    PrintBooks(books);
    return 0;
  }

  Dashboard dashboard(std::move(books), config.project_root);
  return dashboard.Run();
}

}  // namespace wordrush
