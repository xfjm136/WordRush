#include "wordrush/types.hpp"

#include <algorithm>
#include <cctype>

namespace wordrush {

namespace {

std::string Lower(std::string_view text) {
  std::string lowered(text);
  std::transform(lowered.begin(), lowered.end(), lowered.begin(), [](unsigned char ch) {
    return static_cast<char>(std::tolower(ch));
  });
  return lowered;
}

}  // namespace

std::string ToString(WordItemKind kind) {
  switch (kind) {
    case WordItemKind::Word:
      return "word";
    case WordItemKind::Phrase:
      return "phrase";
    case WordItemKind::Collocation:
      return "collocation";
    case WordItemKind::Cloze:
      return "cloze";
  }

  return "word";
}

WordItemKind ParseWordItemKind(std::string_view raw) {
  const std::string normalized = Lower(Trim(raw));
  if (normalized == "phrase") {
    return WordItemKind::Phrase;
  }
  if (normalized == "collocation") {
    return WordItemKind::Collocation;
  }
  if (normalized == "cloze") {
    return WordItemKind::Cloze;
  }
  return WordItemKind::Word;
}

std::size_t BookProgress::studied_items() const {
  return learning_items + review_items + mastered_items;
}

double BookProgress::mastery_ratio(std::size_t total_entries) const {
  if (total_entries == 0) {
    return 0.0;
  }
  return static_cast<double>(mastered_items) / static_cast<double>(total_entries);
}

double BookProgress::urgency_ratio(std::size_t total_entries) const {
  if (total_entries == 0) {
    return 0.0;
  }
  return static_cast<double>(due_now) / static_cast<double>(total_entries);
}

std::string Trim(std::string_view text) {
  std::size_t begin = 0;
  while (begin < text.size() && std::isspace(static_cast<unsigned char>(text[begin])) != 0) {
    ++begin;
  }

  std::size_t end = text.size();
  while (end > begin && std::isspace(static_cast<unsigned char>(text[end - 1])) != 0) {
    --end;
  }

  return std::string(text.substr(begin, end - begin));
}

std::vector<std::string> Split(std::string_view text, char delimiter) {
  std::vector<std::string> parts;
  std::size_t cursor = 0;
  while (cursor <= text.size()) {
    const std::size_t found = text.find(delimiter, cursor);
    if (found == std::string_view::npos) {
      parts.emplace_back(text.substr(cursor));
      break;
    }
    parts.emplace_back(text.substr(cursor, found - cursor));
    cursor = found + 1;
  }
  return parts;
}

std::string Join(const std::vector<std::string>& items, std::string_view delimiter) {
  std::string joined;
  for (std::size_t i = 0; i < items.size(); ++i) {
    if (i != 0) {
      joined += delimiter;
    }
    joined += items[i];
  }
  return joined;
}

}  // namespace wordrush

