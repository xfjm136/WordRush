#include "wordrush/drill_session.hpp"

#include <algorithm>
#include <cctype>
#include <random>
#include <string>
#include <utility>
#include <vector>

#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

#include "wordrush/companion.hpp"

namespace wordrush {

namespace {

using namespace ftxui;

struct Palette {
  Color background = Color::RGB(30, 30, 46);
  Color surface = Color::RGB(49, 50, 68);
  Color surface_focus = Color::RGB(69, 71, 90);
  Color text = Color::RGB(205, 214, 244);
  Color muted = Color::RGB(166, 173, 200);
  Color subtle = Color::RGB(127, 132, 156);
  Color success = Color::RGB(166, 227, 161);
  Color warning = Color::RGB(250, 179, 135);
  Color info = Color::RGB(137, 180, 250);
  Color accent = Color::RGB(203, 166, 247);
};

Palette Theme() {
  return {};
}

std::string Lower(std::string_view text) {
  std::string lowered(text);
  std::transform(lowered.begin(), lowered.end(), lowered.begin(), [](unsigned char ch) {
    return static_cast<char>(std::tolower(ch));
  });
  return lowered;
}

std::string NormalizeAnswer(std::string text) {
  text = Trim(text);
  std::transform(text.begin(), text.end(), text.begin(), [](unsigned char ch) {
    return static_cast<char>(std::tolower(ch));
  });
  return text;
}

std::string ModeTitle(DrillMode mode, RecallVariant recall_variant) {
  switch (mode) {
    case DrillMode::Scan:
      return "速刷";
    case DrillMode::TypeRecall:
      return "拼写回忆 · " + ToString(recall_variant);
    case DrillMode::MistakeBurst:
      return "错词回捶";
  }

  return "训练";
}

std::vector<WordBookItem> FilterItems(const WordBookDetail& book, DrillMode mode) {
  std::vector<WordBookItem> items;
  items.reserve(book.items.size());

  for (const auto& item : book.items) {
    if (mode == DrillMode::TypeRecall || mode == DrillMode::MistakeBurst) {
      if (!item.term.empty()) {
        items.push_back(item);
      }
      continue;
    }

    items.push_back(item);
  }

  if (items.empty()) {
    items = book.items;
  }

  std::mt19937 rng(42);
  std::shuffle(items.begin(), items.end(), rng);

  if (mode == DrillMode::MistakeBurst && items.size() > 12) {
    items.resize(12);
  } else if (mode == DrillMode::Scan && items.size() > 20) {
    items.resize(20);
  } else if (mode == DrillMode::TypeRecall && items.size() > 20) {
    items.resize(20);
  }

  return items;
}

std::string PrimaryMeaning(const WordBookItem& item) {
  if (item.meaning.empty()) {
    return "";
  }

  const auto slash = item.meaning.find_first_of("；;/");
  if (slash == std::string::npos) {
    return item.meaning;
  }
  return Trim(item.meaning.substr(0, slash));
}

Element StatChip(const std::string& label, const std::string& value, const Palette& palette, Color tone) {
  return vbox({
             text(label) | color(palette.subtle),
             text(value) | bold | color(tone),
         }) |
         borderRounded | bgcolor(palette.surface);
}

Element TinyPanel(const std::string& title, Element body, const Palette& palette) {
  return vbox({
             text(title) | bold | color(palette.text),
             separator(),
             std::move(body),
         }) |
         borderRounded | bgcolor(palette.surface);
}

std::string BuildPrompt(const WordBookItem& item, DrillMode mode, RecallVariant recall_variant) {
  if (mode == DrillMode::Scan) {
    return item.term;
  }

  if (mode == DrillMode::MistakeBurst || recall_variant == RecallVariant::CnToEn) {
    if (!item.meaning.empty()) {
      return item.meaning;
    }
  }

  if (recall_variant == RecallVariant::EnToCn) {
    return item.term;
  }

  if (recall_variant == RecallVariant::Mixed) {
    if ((item.term.size() + item.meaning.size()) % 2 == 0) {
      return item.term;
    }
    return item.meaning;
  }

  if (!item.meaning.empty()) {
    return item.meaning;
  }

  return item.example;
}

std::string BuildHint(const WordBookItem& item, DrillMode mode, RecallVariant recall_variant) {
  if (mode == DrillMode::Scan) {
    return item.meaning;
  }

  if (recall_variant == RecallVariant::EnToCn) {
    if (!item.pronunciation.empty()) {
      return item.pronunciation;
    }
    return "输入中文释义";
  }

  std::string hint;
  if (!item.pronunciation.empty()) {
    hint += item.pronunciation;
  }
  if (!item.phrases.empty()) {
    if (!hint.empty()) {
      hint += "  ·  ";
    }
    hint += item.phrases;
  }
  if (hint.empty()) {
    hint = "直接回忆英文并输入";
  }
  return hint;
}

bool CheckAnswer(const WordBookItem& item, DrillMode mode, RecallVariant recall_variant, const std::string& input) {
  const std::string normalized = NormalizeAnswer(input);
  if (mode == DrillMode::Scan) {
    return true;
  }

  if (mode == DrillMode::MistakeBurst || recall_variant == RecallVariant::CnToEn) {
    return normalized == NormalizeAnswer(item.term);
  }

  if (recall_variant == RecallVariant::EnToCn) {
    const std::string primary = NormalizeAnswer(PrimaryMeaning(item));
    return !primary.empty() && normalized == primary;
  }

  if (recall_variant == RecallVariant::Mixed) {
    if ((item.term.size() + item.meaning.size()) % 2 == 0) {
      const std::string primary = NormalizeAnswer(PrimaryMeaning(item));
      return !primary.empty() && normalized == primary;
    }
    return normalized == NormalizeAnswer(item.term);
  }

  return normalized == NormalizeAnswer(item.term);
}

std::string RevealAnswer(const WordBookItem& item, DrillMode mode, RecallVariant recall_variant) {
  if (mode == DrillMode::MistakeBurst || recall_variant == RecallVariant::CnToEn) {
    return item.term;
  }

  if (recall_variant == RecallVariant::EnToCn || recall_variant == RecallVariant::Mixed) {
    return item.meaning;
  }

  return item.term;
}

}  // namespace

DrillSession::DrillSession(WordBookDetail book, DrillMode mode, RecallVariant recall_variant)
    : book_(std::move(book)), mode_(mode), recall_variant_(recall_variant) {}

int DrillSession::Run() {
  using namespace ftxui;

  const Palette palette = Theme();
  std::vector<WordBookItem> items = FilterItems(book_, mode_);
  if (items.empty()) {
    return 0;
  }

  ScreenInteractive screen = ScreenInteractive::Fullscreen();
  std::size_t index = 0;
  int correct = 0;
  int wrong = 0;
  bool revealed = false;
  bool finished = false;
  std::string feedback;
  std::string input_value;

  InputOption input_option;
  input_option.placeholder = "在这里输入答案，按 Enter 提交";
  Component input = Input(&input_value, input_option);

  auto renderer = Renderer(input, [&] {
    if (finished || index >= items.size()) {
      const CompanionState companion = BuildCompanionState(book_, correct, wrong, index);
      return vbox({
                 text(book_.meta.title) | bold,
                 separator(),
                 text("本轮训练完成，正在返回。") | color(palette.success),
                 separator(),
                 text(companion.speech) | color(palette.text),
             }) |
             borderRounded | bgcolor(palette.background) | color(palette.text) | size(WIDTH, GREATER_THAN, 56);
    }

    const WordBookItem& item = items[index];
    const std::string prompt = BuildPrompt(item, mode_, recall_variant_);
    const std::string hint = BuildHint(item, mode_, recall_variant_);
    const CompanionState companion = BuildCompanionState(book_, correct, wrong, index);

    Element answer_box = TinyPanel(
        "输入答案",
        vbox({
            input->Render(),
        }),
        palette);

    Element reveal_box = TinyPanel(
        "正确答案",
        vbox({
            text(RevealAnswer(item, mode_, recall_variant_)) | bold | color(palette.success),
            text(item.term + "  ·  " + item.meaning) | color(palette.muted),
        }),
        palette);

    Element prompt_panel = TinyPanel(
        "题目",
        vbox({
            text(prompt) | bold | color(palette.text),
            text(hint) | color(palette.muted),
            separator(),
            text("例句") | color(palette.subtle),
            text(item.example.empty() ? "无" : item.example) | color(palette.muted),
        }),
        palette);

    Element companion_panel = TinyPanel(
        companion.name + " · " + companion.mood,
        vbox({
            text(companion.ascii_art) | color(palette.text),
            text(companion.speech) | color(palette.muted),
        }),
        palette);

    Elements body;
    body.push_back(hbox({
        text(book_.meta.title) | bold,
        filler(),
        text(ModeTitle(mode_, recall_variant_)) | color(palette.accent),
    }));
    body.push_back(separator());
    body.push_back(hbox({
        StatChip("进度", std::to_string(index + 1) + "/" + std::to_string(items.size()), palette, palette.info),
        StatChip("答对", std::to_string(correct), palette, palette.success),
        StatChip("答错", std::to_string(wrong), palette, palette.warning),
    }));
    body.push_back(separator());
    body.push_back(hbox({
        prompt_panel | flex,
        separator(),
        companion_panel | size(WIDTH, EQUAL, 28),
    }));
    body.push_back(separator());
    body.push_back(answer_box);

    if (!feedback.empty()) {
      body.push_back(separator());
      body.push_back(text(feedback) | color(revealed ? palette.warning : palette.success));
    }

    if (revealed) {
      body.push_back(separator());
      body.push_back(reveal_box);
    }

    body.push_back(separator());
    body.push_back(text("Enter 提交    Tab 看答案 / 下一题    Esc 返回") | color(palette.muted));

    return vbox(std::move(body)) | borderRounded | bgcolor(palette.background) | color(palette.text) |
           size(WIDTH, GREATER_THAN, 76);
  });

  auto app = CatchEvent(renderer, [&](Event event) {
    if (event == Event::Escape) {
      screen.Exit();
      return true;
    }

    if (finished || index >= items.size()) {
      screen.Exit();
      return true;
    }

    if (event == Event::Tab) {
      if (!revealed) {
        revealed = true;
        feedback = "已显示答案。看一眼后继续下一题。";
        return true;
      }

      revealed = false;
      feedback.clear();
      input_value.clear();
      ++index;
      if (index >= items.size()) {
        finished = true;
        screen.Exit();
      }
      return true;
    }

    if (event == Event::Return) {
      if (revealed) {
        revealed = false;
        feedback.clear();
        input_value.clear();
        ++index;
        if (index >= items.size()) {
          finished = true;
          screen.Exit();
        }
        return true;
      }

      const WordBookItem& item = items[index];
      if (mode_ == DrillMode::Scan) {
        ++correct;
        feedback = "已记过一遍，继续下一题。";
        revealed = true;
        return true;
      }

      const bool ok = CheckAnswer(item, mode_, recall_variant_, input_value);
      if (ok) {
        ++correct;
        feedback = "正确，继续保持这个节奏。";
        revealed = true;
      } else {
        ++wrong;
        feedback = "没答对，先看答案，再继续。";
        revealed = true;
      }
      return true;
    }

    return input->OnEvent(event);
  });

  screen.Loop(app);
  return 0;
}

}  // namespace wordrush
