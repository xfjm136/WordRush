#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "wordrush/types.hpp"

namespace wordrush {

class WordBookRepository {
 public:
  explicit WordBookRepository(std::filesystem::path project_root);

  [[nodiscard]] std::vector<WordBookOverview> LoadOverview() const;
  [[nodiscard]] WordBookDetail LoadBookDetail(const std::string& book_id) const;
  bool ImportBook(const std::filesystem::path& source_path, std::string* error = nullptr) const;
  bool DeleteBook(const std::string& book_id, std::string* error = nullptr) const;
  bool MergeBooks(const std::vector<std::string>& book_ids,
                  const std::string& new_id,
                  const std::string& new_title,
                  std::string* error = nullptr) const;
  bool SplitBook(const std::string& book_id,
                 const std::string& tag,
                 std::vector<std::string>* created_ids = nullptr,
                 std::string* error = nullptr) const;

 private:
  [[nodiscard]] std::vector<WordBookMeta> LoadWordBooks() const;
  [[nodiscard]] WordBookMeta ParseWordBook(const std::filesystem::path& file) const;
  [[nodiscard]] std::vector<WordBookItem> LoadWordBookItems(const std::filesystem::path& file) const;
  [[nodiscard]] std::filesystem::path ResolveBookPath(const std::string& book_id) const;
  bool WriteBook(const WordBookDetail& detail, std::string* error = nullptr) const;
  bool UpsertProgress(const std::string& book_id, std::size_t total_entries, std::string* error = nullptr) const;
  bool RemoveProgress(const std::string& book_id, std::string* error = nullptr) const;

  std::filesystem::path project_root_;
  std::filesystem::path books_dir_;
  std::filesystem::path progress_file_;
};

}  // namespace wordrush
