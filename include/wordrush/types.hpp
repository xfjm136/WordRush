#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace wordrush {

enum class WordItemKind {
  Word,
  Phrase,
  Collocation,
  Cloze,
};

std::string ToString(WordItemKind kind);
WordItemKind ParseWordItemKind(std::string_view raw);

struct WordEntry {
  WordItemKind kind = WordItemKind::Word;
  std::string term;
  std::string pronunciation;
  std::string meaning;
  std::string phrases;
  std::string example;
  std::vector<std::string> tags;
};

struct WordBookMeta {
  std::string id;
  std::string title;
  std::string exam;
  std::string track;
  std::string description;
  std::string accent;
  std::size_t total_entries = 0;
  std::size_t phrase_entries = 0;
};

struct BookProgress {
  std::size_t new_items = 0;
  std::size_t learning_items = 0;
  std::size_t review_items = 0;
  std::size_t mastered_items = 0;
  std::size_t due_now = 0;
  double accuracy = 0.0;
  int streak_days = 0;
  int estimated_minutes = 0;
  std::string last_trained;

  [[nodiscard]] std::size_t studied_items() const;
  [[nodiscard]] double mastery_ratio(std::size_t total_entries) const;
  [[nodiscard]] double urgency_ratio(std::size_t total_entries) const;
};

struct WordBookOverview {
  WordBookMeta meta;
  BookProgress progress;
};

struct WordBookItem {
  WordItemKind kind = WordItemKind::Word;
  std::string term;
  std::string pronunciation;
  std::string meaning;
  std::string phrases;
  std::string example;
  std::vector<std::string> tags;
};

struct WordBookDetail {
  WordBookMeta meta;
  BookProgress progress;
  std::vector<WordBookItem> items;
};

std::string Trim(std::string_view text);
std::vector<std::string> Split(std::string_view text, char delimiter);
std::string Join(const std::vector<std::string>& items, std::string_view delimiter);

}  // namespace wordrush
