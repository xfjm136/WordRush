#include "wordrush/storage.hpp"

#include <algorithm>
#include <fstream>
#include <set>
#include <filesystem>
#include <sstream>
#include <string>
#include <unordered_map>

namespace wordrush {

namespace {

std::string Lower(std::string_view text) {
  std::string lowered(text);
  std::transform(lowered.begin(), lowered.end(), lowered.begin(), [](unsigned char ch) {
    return static_cast<char>(std::tolower(ch));
  });
  return lowered;
}

std::size_t ParseSize(std::string_view raw) {
  const std::string trimmed = Trim(raw);
  if (trimmed.empty()) {
    return 0;
  }
  return static_cast<std::size_t>(std::stoull(trimmed));
}

int ParseInt(std::string_view raw) {
  const std::string trimmed = Trim(raw);
  if (trimmed.empty()) {
    return 0;
  }
  return std::stoi(trimmed);
}

double ParseDouble(std::string_view raw) {
  const std::string trimmed = Trim(raw);
  if (trimmed.empty()) {
    return 0.0;
  }
  return std::stod(trimmed);
}

std::unordered_map<std::string, BookProgress> LoadProgress(const std::filesystem::path& progress_file) {
  std::unordered_map<std::string, BookProgress> progress_map;

  std::ifstream input(progress_file);
  if (!input) {
    return progress_map;
  }

  std::string line;
  bool header_skipped = false;
  while (std::getline(input, line)) {
    line = Trim(line);
    if (line.empty() || line.starts_with('#')) {
      continue;
    }

    if (!header_skipped) {
      header_skipped = true;
      continue;
    }

    const auto columns = Split(line, '\t');
    if (columns.size() < 10) {
      continue;
    }

    BookProgress progress;
    progress.new_items = ParseSize(columns[1]);
    progress.learning_items = ParseSize(columns[2]);
    progress.review_items = ParseSize(columns[3]);
    progress.mastered_items = ParseSize(columns[4]);
    progress.due_now = ParseSize(columns[5]);
    progress.accuracy = ParseDouble(columns[6]);
    progress.streak_days = ParseInt(columns[7]);
    progress.estimated_minutes = ParseInt(columns[8]);
    progress.last_trained = Trim(columns[9]);

    progress_map.emplace(Trim(columns[0]), progress);
  }

  return progress_map;
}

void AppendOrReplaceProgressLine(std::ofstream& output,
                                 const std::string& book_id,
                                 std::size_t total_entries,
                                 const BookProgress& progress) {
  output << book_id << '\t' << progress.new_items << '\t' << progress.learning_items << '\t'
         << progress.review_items << '\t' << progress.mastered_items << '\t' << progress.due_now << '\t'
         << progress.accuracy << '\t' << progress.streak_days << '\t' << progress.estimated_minutes << '\t'
         << progress.last_trained << '\n';
}

std::string Slugify(std::string value) {
  value = Lower(Trim(value));
  for (char& ch : value) {
    const unsigned char byte = static_cast<unsigned char>(ch);
    if (std::isalnum(byte) == 0) {
      ch = '-';
    }
  }
  while (!value.empty() && value.front() == '-') {
    value.erase(value.begin());
  }
  while (!value.empty() && value.back() == '-') {
    value.pop_back();
  }
  if (value.empty()) {
    value = "wordbook";
  }
  return value;
}

}  // namespace

WordBookRepository::WordBookRepository(std::filesystem::path project_root)
    : project_root_(std::move(project_root)),
      books_dir_(project_root_ / "data" / "library" / "books"),
      progress_file_(project_root_ / "data" / "state" / "progress.tsv") {}

std::filesystem::path WordBookRepository::ResolveBookPath(const std::string& book_id) const {
  return books_dir_ / (book_id + ".tsv");
}

std::vector<WordBookOverview> WordBookRepository::LoadOverview() const {
  const auto metas = LoadWordBooks();
  const auto progress_map = LoadProgress(progress_file_);

  std::vector<WordBookOverview> overview;
  overview.reserve(metas.size());
  for (const auto& meta : metas) {
    BookProgress progress;
    if (const auto it = progress_map.find(meta.id); it != progress_map.end()) {
      progress = it->second;
    }

    overview.push_back({.meta = meta, .progress = progress});
  }

  std::sort(overview.begin(), overview.end(), [](const WordBookOverview& left, const WordBookOverview& right) {
    if (left.progress.due_now != right.progress.due_now) {
      return left.progress.due_now > right.progress.due_now;
    }
    return left.meta.title < right.meta.title;
  });

  return overview;
}

WordBookDetail WordBookRepository::LoadBookDetail(const std::string& book_id) const {
  WordBookDetail detail;
  const std::filesystem::path file = ResolveBookPath(book_id);
  detail.meta = ParseWordBook(file);
  detail.items = LoadWordBookItems(file);
  return detail;
}

bool WordBookRepository::WriteBook(const WordBookDetail& detail, std::string* error) const {
  std::error_code ec;
  std::filesystem::create_directories(books_dir_, ec);
  if (ec) {
    if (error) {
      *error = "无法创建词本目录";
    }
    return false;
  }

  const std::filesystem::path file = ResolveBookPath(detail.meta.id);
  std::ofstream output(file, std::ios::trunc);
  if (!output) {
    if (error) {
      *error = "无法写入词本文件";
    }
    return false;
  }

  output << "# id: " << detail.meta.id << '\n';
  output << "# title: " << detail.meta.title << '\n';
  output << "# exam: " << detail.meta.exam << '\n';
  output << "# track: " << detail.meta.track << '\n';
  output << "# description: " << detail.meta.description << '\n';
  output << "# accent: " << detail.meta.accent << '\n';
  output << "kind\tterm\tpronunciation\tmeaning\tphrases\texample\ttags\n";

  for (const auto& item : detail.items) {
    output << ToString(item.kind) << '\t' << item.term << '\t' << item.pronunciation << '\t' << item.meaning
           << '\t' << item.phrases << '\t' << item.example << '\t' << Join(item.tags, ",") << '\n';
  }

  if (!output) {
    if (error) {
      *error = "写入词本时发生错误";
    }
    return false;
  }

  return true;
}

bool WordBookRepository::UpsertProgress(const std::string& book_id, std::size_t total_entries, std::string* error) const {
  std::error_code ec;
  std::filesystem::create_directories(progress_file_.parent_path(), ec);
  if (ec) {
    if (error) {
      *error = "无法创建进度目录";
    }
    return false;
  }

  auto progress_map = LoadProgress(progress_file_);
  BookProgress progress;
  if (const auto it = progress_map.find(book_id); it != progress_map.end()) {
    progress = it->second;
  }

  progress.new_items = std::min(progress.new_items, total_entries);
  progress.learning_items = std::min(progress.learning_items, total_entries);
  progress.review_items = std::min(progress.review_items, total_entries);
  progress.mastered_items = std::min(progress.mastered_items, total_entries);
  progress.due_now = std::min(progress.due_now, total_entries);

  std::ofstream output(progress_file_, std::ios::trunc);
  if (!output) {
    if (error) {
      *error = "无法写入进度文件";
    }
    return false;
  }

  output << "book_id\tnew_items\tlearning_items\treview_items\tmastered_items\tdue_now\taccuracy\tstreak_days\t"
            "estimated_minutes\tlast_trained\n";

  bool written = false;
  for (const auto& [id, item] : progress_map) {
    if (id == book_id) {
      AppendOrReplaceProgressLine(output, book_id, total_entries, progress);
      written = true;
    } else {
      AppendOrReplaceProgressLine(output, id, total_entries, item);
    }
  }

  if (!written) {
    AppendOrReplaceProgressLine(output, book_id, total_entries, progress);
  }

  return static_cast<bool>(output);
}

bool WordBookRepository::RemoveProgress(const std::string& book_id, std::string* error) const {
  auto progress_map = LoadProgress(progress_file_);
  if (progress_map.empty()) {
    return true;
  }

  progress_map.erase(book_id);
  std::error_code ec;
  std::filesystem::create_directories(progress_file_.parent_path(), ec);
  if (ec) {
    if (error) {
      *error = "无法创建进度目录";
    }
    return false;
  }

  std::ofstream output(progress_file_, std::ios::trunc);
  if (!output) {
    if (error) {
      *error = "无法写入进度文件";
    }
    return false;
  }

  output << "book_id\tnew_items\tlearning_items\treview_items\tmastered_items\tdue_now\taccuracy\tstreak_days\t"
            "estimated_minutes\tlast_trained\n";
  for (const auto& [id, item] : progress_map) {
    AppendOrReplaceProgressLine(output, id, 0, item);
  }

  return static_cast<bool>(output);
}

std::vector<WordBookMeta> WordBookRepository::LoadWordBooks() const {
  std::vector<WordBookMeta> books;

  if (!std::filesystem::exists(books_dir_)) {
    return books;
  }

  for (const auto& entry : std::filesystem::directory_iterator(books_dir_)) {
    if (!entry.is_regular_file() || entry.path().extension() != ".tsv") {
      continue;
    }
    books.push_back(ParseWordBook(entry.path()));
  }

  std::sort(books.begin(), books.end(), [](const WordBookMeta& left, const WordBookMeta& right) {
    return left.title < right.title;
  });

  return books;
}

WordBookMeta WordBookRepository::ParseWordBook(const std::filesystem::path& file) const {
  WordBookMeta meta;
  meta.id = file.stem().string();
  meta.title = file.stem().string();
  meta.exam = "General";
  meta.track = "Vocabulary";
  meta.description = "Practice book";
  meta.accent = "linen";

  std::ifstream input(file);
  if (!input) {
    return meta;
  }

  bool header_skipped = false;
  std::string line;
  while (std::getline(input, line)) {
    line = Trim(line);
    if (line.empty()) {
      continue;
    }

    if (line.starts_with('#')) {
      const std::string payload = Trim(line.substr(1));
      const std::size_t delimiter = payload.find(':');
      if (delimiter != std::string::npos) {
        const std::string key = Lower(Trim(payload.substr(0, delimiter)));
        const std::string value = Trim(payload.substr(delimiter + 1));
        if (key == "id") {
          meta.id = value;
        } else if (key == "title") {
          meta.title = value;
        } else if (key == "exam") {
          meta.exam = value;
        } else if (key == "track") {
          meta.track = value;
        } else if (key == "description") {
          meta.description = value;
        } else if (key == "accent") {
          meta.accent = value;
        }
      }
      continue;
    }

    if (!header_skipped) {
      header_skipped = true;
      continue;
    }

    const auto columns = Split(line, '\t');
    if (columns.size() < 6) {
      continue;
    }

    ++meta.total_entries;
    const auto kind = ParseWordItemKind(columns[0]);
    if (kind != WordItemKind::Word || !Trim(columns[4]).empty()) {
      ++meta.phrase_entries;
    }
  }

  return meta;
}

std::vector<WordBookItem> WordBookRepository::LoadWordBookItems(const std::filesystem::path& file) const {
  std::vector<WordBookItem> items;

  std::ifstream input(file);
  if (!input) {
    return items;
  }

  bool header_seen = false;
  std::string line;
  while (std::getline(input, line)) {
    line = Trim(line);
    if (line.empty() || line.starts_with('#')) {
      continue;
    }

    if (!header_seen) {
      header_seen = true;
      continue;
    }

    const auto columns = Split(line, '\t');
    if (columns.size() < 7) {
      continue;
    }

    WordBookItem item;
    item.kind = ParseWordItemKind(columns[0]);
    item.term = Trim(columns[1]);
    item.pronunciation = Trim(columns[2]);
    item.meaning = Trim(columns[3]);
    item.phrases = Trim(columns[4]);
    item.example = Trim(columns[5]);
    item.tags = Split(columns[6], ',');
    for (auto& tag : item.tags) {
      tag = Trim(tag);
    }

    items.push_back(std::move(item));
  }

  return items;
}

bool WordBookRepository::ImportBook(const std::filesystem::path& source_path, std::string* error) const {
  if (!std::filesystem::exists(source_path)) {
    if (error) {
      *error = "源文件不存在";
    }
    return false;
  }

  WordBookDetail detail;
  detail.meta = ParseWordBook(source_path);
  detail.items = LoadWordBookItems(source_path);
  if (detail.meta.id.empty()) {
    detail.meta.id = Slugify(detail.meta.title.empty() ? source_path.stem().string() : detail.meta.title);
  }
  if (detail.meta.title.empty()) {
    detail.meta.title = detail.meta.id;
  }

  if (!WriteBook(detail, error)) {
    return false;
  }

  return UpsertProgress(detail.meta.id, detail.items.size(), error);
}

bool WordBookRepository::DeleteBook(const std::string& book_id, std::string* error) const {
  std::error_code ec;
  std::filesystem::remove(ResolveBookPath(book_id), ec);
  if (ec) {
    if (error) {
      *error = "删除词本失败";
    }
    return false;
  }
  return RemoveProgress(book_id, error);
}

bool WordBookRepository::MergeBooks(const std::vector<std::string>& book_ids,
                                    const std::string& new_id,
                                    const std::string& new_title,
                                    std::string* error) const {
  if (book_ids.empty()) {
    if (error) {
      *error = "没有可合并的词本";
    }
    return false;
  }

  WordBookDetail merged;
  merged.meta.id = Slugify(new_id.empty() ? new_title : new_id);
  merged.meta.title = new_title.empty() ? merged.meta.id : new_title;
  merged.meta.exam = "Merged";
  merged.meta.track = "Merged";
  merged.meta.description = "Merged wordbook";
  merged.meta.accent = "cobalt";

  for (const auto& id : book_ids) {
    const WordBookDetail detail = LoadBookDetail(id);
    if (detail.items.empty()) {
      continue;
    }
    merged.items.insert(merged.items.end(), detail.items.begin(), detail.items.end());
  }

  if (merged.items.empty()) {
    if (error) {
      *error = "合并后的词条为空";
    }
    return false;
  }

  merged.meta.total_entries = merged.items.size();
  return WriteBook(merged, error) && UpsertProgress(merged.meta.id, merged.meta.total_entries, error);
}

bool WordBookRepository::SplitBook(const std::string& book_id,
                                   const std::string& tag,
                                   std::vector<std::string>* created_ids,
                                   std::string* error) const {
  const WordBookDetail source = LoadBookDetail(book_id);
  if (source.items.empty()) {
    if (error) {
      *error = "词本为空，无法拆分";
    }
    return false;
  }

  std::unordered_map<std::string, WordBookDetail> groups;
  for (const auto& item : source.items) {
    bool matched = false;
    for (const auto& item_tag : item.tags) {
      if (item_tag == tag) {
        matched = true;
        break;
      }
    }
    if (!matched) {
      continue;
    }

    WordBookDetail& bucket = groups[tag];
    if (bucket.meta.id.empty()) {
      bucket.meta.id = Slugify(source.meta.id + "-" + tag);
      bucket.meta.title = source.meta.title + " · " + tag;
      bucket.meta.exam = source.meta.exam;
      bucket.meta.track = source.meta.track;
      bucket.meta.description = source.meta.description;
      bucket.meta.accent = source.meta.accent;
    }
    bucket.items.push_back(item);
  }

  if (groups.empty()) {
    if (error) {
      *error = "没有找到可拆分的标签";
    }
    return false;
  }

  for (auto& [_, detail] : groups) {
    detail.meta.total_entries = detail.items.size();
    if (!WriteBook(detail, error) || !UpsertProgress(detail.meta.id, detail.meta.total_entries, error)) {
      return false;
    }
    if (created_ids) {
      created_ids->push_back(detail.meta.id);
    }
  }

  return true;
}

}  // namespace wordrush
