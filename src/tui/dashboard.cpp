#include "wordrush/dashboard.hpp"

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

#include "wordrush/companion.hpp"
#include "wordrush/drill_session.hpp"
#include "wordrush/settings.hpp"
#include "wordrush/theme.hpp"

namespace wordrush {

namespace {

using namespace ftxui;

using Palette = ThemePalette;

Color AccentColor(const std::string& accent) {
  if (accent == "amber") return Color::RGB(249, 226, 175);
  if (accent == "teal") return Color::RGB(148, 226, 213);
  if (accent == "crimson") return Color::RGB(243, 139, 168);
  if (accent == "cobalt") return Color::RGB(137, 180, 250);
  return Color::RGB(180, 190, 254);
}

std::string Percent(double value) {
  return std::to_string(static_cast<int>(value * 100.0)) + "%";
}

struct ShortcutSpec {
  char key;
  DrillMode mode;
  RecallVariant variant;
  std::string title;
  std::string detail;
  Color tone;
};

std::vector<ShortcutSpec> BuildShortcuts(const Palette& palette) {
  return {
      {'1', DrillMode::Scan, RecallVariant::CnToEn, "1 速刷", "先扫一遍，快速预热", palette.blue},
      {'2', DrillMode::TypeRecall, RecallVariant::CnToEn, "2 拼写回忆", "中译英", palette.green},
      {'3', DrillMode::TypeRecall, RecallVariant::EnToCn, "3 拼写回忆", "英译中", palette.peach},
      {'4', DrillMode::TypeRecall, RecallVariant::Mixed, "4 拼写回忆", "混译", palette.blue},
      {'5', DrillMode::MistakeBurst, RecallVariant::Mixed, "5 错词回捶", "反复回练薄弱项", palette.peach},
  };
}

Element Section(const std::string& title, Element body, const Palette& palette) {
  Element section = vbox({
             text(title) | bold | color(palette.text),
             separator(),
             std::move(body),
         }) |
         borderRounded;
  if (palette.use_background) {
    section = section | bgcolor(palette.surface);
  }
  return section;
}

Element BookRow(const WordBookOverview& book, bool selected, const Palette& palette) {
  const Color accent = AccentColor(book.meta.accent);
  Element row = vbox({
                    hbox({
                        text(selected ? "● " : "○ ") | color(accent),
                        text(book.meta.title) | bold,
                        filler(),
                        text(std::to_string(book.progress.due_now)) | color(palette.peach),
                    }),
                    text(book.meta.exam) | color(palette.muted),
                }) |
                borderRounded;

  if (palette.use_background) {
    return selected ? row | bgcolor(palette.focus) | color(palette.text)
                    : row | bgcolor(palette.surface_alt) | color(palette.text);
  }
  return selected ? row | bold | color(palette.text) : row | color(palette.text);
}

Element StatLine(const std::string& label, const std::string& value, const Palette& palette, Color tone) {
  return hbox({
             text(label) | color(palette.muted),
             filler(),
             text(value) | bold | color(tone),
         });
}

Element ShortcutLine(const ShortcutSpec& spec, const Palette& palette) {
  return hbox({
             text(spec.title) | color(spec.tone),
             filler(),
             text(spec.detail) | color(palette.subtle),
         });
}

Element CompanionCard(const CompanionState& companion, const Palette& palette) {
  std::string action_line = companion.actions.empty() ? "陪你背词" : companion.actions.front();
  Element card = vbox({
             hbox({
                 text(companion.name) | bold | color(palette.green),
                 filler(),
                 text("Lv." + std::to_string(companion.level)) | color(palette.blue),
             }),
             text(companion.ascii_art) | color(palette.text),
             text(companion.mood + " · " + action_line) | color(palette.muted),
             separator(),
             text(companion.speech) | color(palette.text),
         }) |
         borderRounded | color(palette.text);
  if (palette.use_background) {
    card = card | bgcolor(palette.surface_alt);
  }
  return card;
}

Element HelpModal(const Palette& palette) {
  Element modal = vbox({
             text("帮助") | bold,
             separator(),
             text("j / k: 切换词本"),
             text("1: 速刷"),
             text("2: 中译英"),
             text("3: 英译中"),
             text("4: 混译"),
             text("5: 错词回捶"),
             text("m: 管理词本"),
             text("Enter: 看今天建议"),
             text("q / Esc: 返回"),
         }) |
         borderRounded | color(palette.text) | size(WIDTH, EQUAL, 44);
  if (palette.use_background) {
    modal = modal | bgcolor(palette.focus);
  }
  return modal;
}

Element PlanModal(const WordBookOverview& book,
                  const std::vector<ShortcutSpec>& shortcuts,
                  const SessionRecommendation& plan,
                  const Palette& palette) {
  Elements lines;
  lines.push_back(text(book.meta.title) | bold);
  lines.push_back(text("直接按数字开始") | color(palette.muted));
  lines.push_back(separator());
  for (const auto& shortcut : shortcuts) {
    lines.push_back(ShortcutLine(shortcut, palette));
  }
  if (!plan.headline.empty()) {
    lines.push_back(separator());
    lines.push_back(text(plan.headline) | color(palette.text));
  }
  lines.push_back(separator());
  lines.push_back(text("Esc / q 返回") | color(palette.green));
  Element modal = vbox(std::move(lines)) | borderRounded | color(palette.text) | size(WIDTH, EQUAL, 52);
  if (palette.use_background) {
    modal = modal | bgcolor(palette.focus);
  }
  return modal;
}

std::string ReadLine(const std::string& prompt) {
  std::cout << prompt;
  std::string line;
  std::getline(std::cin, line);
  return Trim(line);
}

void PrintInfo(const std::string& message) {
  std::cout << message << '\n';
}

}  // namespace

Dashboard::Dashboard(std::vector<WordBookOverview> books, std::filesystem::path project_root)
    : books_(std::move(books)), project_root_(std::move(project_root)) {}

int Dashboard::Run() {
  using namespace ftxui;

  enum class Overlay {
    None,
    Help,
    Plan,
  };

  UiSettings ui_settings = LoadUiSettings(project_root_);
  std::size_t selected = 0;

  auto refresh = [&] {
    WordBookRepository repository(project_root_);
    books_ = repository.LoadOverview();
    if (books_.empty()) {
      selected = 0;
    } else {
      selected = std::min(selected, books_.size() - 1);
    }
  };

  auto manage_books = [&] {
    WordBookRepository repository(project_root_);
    std::cout << "\n词本管理\n";
    if (!books_.empty()) {
      std::cout << "当前词本:\n";
      for (const auto& book : books_) {
        std::cout << "  - " << book.meta.id << " (" << book.meta.title << ")\n";
      }
    }
    std::cout << "1 导入  2 删除  3 合并  4 拆分  回车返回\n";
    const std::string action = ReadLine("选择操作: ");

    if (action == "1") {
      const std::string path = ReadLine("源文件路径: ");
      std::string error;
      if (repository.ImportBook(path, &error)) {
        PrintInfo("导入成功");
      } else {
        PrintInfo("导入失败: " + error);
      }
    } else if (action == "2") {
      const std::string id = ReadLine("词本ID: ");
      std::string error;
      if (repository.DeleteBook(id, &error)) {
        PrintInfo("删除成功");
      } else {
        PrintInfo("删除失败: " + error);
      }
    } else if (action == "3") {
      const std::string ids = ReadLine("词本ID，逗号分隔: ");
      const std::string new_id = ReadLine("新词本ID: ");
      const std::string title = ReadLine("新词本标题: ");
      std::vector<std::string> parts = Split(ids, ',');
      for (auto& part : parts) {
        part = Trim(part);
      }
      std::string error;
      if (repository.MergeBooks(parts, new_id, title, &error)) {
        PrintInfo("合并成功");
      } else {
        PrintInfo("合并失败: " + error);
      }
    } else if (action == "4") {
      const std::string id = ReadLine("词本ID: ");
      const std::string tag = ReadLine("按标签拆分，标签名: ");
      std::string error;
      std::vector<std::string> created_ids;
      if (repository.SplitBook(id, tag, &created_ids, &error)) {
        PrintInfo("拆分成功");
        if (!created_ids.empty()) {
          PrintInfo("新词本: " + Join(created_ids, ", "));
        }
      } else {
        PrintInfo("拆分失败: " + error);
      }
    }

    refresh();
    ReadLine("按回车返回主界面...");
  };

  for (;;) {
    ScreenInteractive screen = ScreenInteractive::Fullscreen();
    Overlay overlay = Overlay::None;
    std::optional<DrillMode> requested_mode;
    std::optional<RecallVariant> requested_variant;
    bool requested_manage = false;

    auto renderer = Renderer([&] {
      const Palette palette = ThemeByIndex(ui_settings.theme_index, ui_settings.use_theme_background);
      const std::vector<ShortcutSpec> shortcuts = BuildShortcuts(palette);

      if (books_.empty()) {
        Element empty = vbox({
                   text("WordRush") | bold | color(palette.blue),
                   separator(),
                   text("还没有词本。") | bold,
                   text("按 m 导入一个词本，或者把 TSV 放到 data/library/books。") | color(palette.muted),
               }) |
               borderRounded | color(palette.text);
        if (palette.use_background) {
          empty = empty | bgcolor(palette.surface);
        }
        return empty;
      }

      selected = std::min(selected, books_.size() - 1);
      const WordBookOverview& current = books_[selected];
      const SessionRecommendation plan = planner_.BuildPlan(current);
      const Color accent = AccentColor(current.meta.accent);
      const CompanionState companion = BuildCompanionState(books_);

      Elements rows;
      for (std::size_t index = 0; index < books_.size(); ++index) {
        rows.push_back(BookRow(books_[index], index == selected, palette));
      }

      Element books_panel = Section("词本", vbox({vbox(std::move(rows)) | yframe}), palette);

      Element current_panel = Section(
          "当前词本",
          vbox({
              hbox({
                  text(current.meta.title) | bold | color(palette.text),
                  filler(),
                  text(current.meta.exam) | color(accent),
              }),
              text(current.meta.track) | color(palette.muted),
              separator(),
              text(current.meta.description) | color(palette.text),
              separator(),
              StatLine("词条总数", std::to_string(current.meta.total_entries), palette, palette.blue),
              StatLine("待复习", std::to_string(current.progress.due_now), palette, palette.peach),
              StatLine("掌握率", Percent(current.progress.mastery_ratio(current.meta.total_entries)), palette,
                       palette.green),
          }),
          palette);

      Elements shortcut_lines;
      for (const auto& shortcut : shortcuts) {
        shortcut_lines.push_back(ShortcutLine(shortcut, palette));
      }

      Element shortcut_panel = Section("开始训练", vbox(std::move(shortcut_lines)), palette);

      Element status_panel = Section(
          "今日状态",
          vbox({
              StatLine("今日待复习", std::to_string(current.progress.due_now), palette, palette.peach),
              StatLine("连续天数", std::to_string(current.progress.streak_days), palette, palette.blue),
              StatLine("预计耗时", std::to_string(current.progress.estimated_minutes) + " 分钟", palette,
                       palette.green),
          }),
          palette);

      Element action_panel = Section(
          "管理词本",
          vbox({
              text("m 导入 / 删除 / 合并 / 拆分") | color(palette.text),
              text("Enter 查看今天建议") | color(palette.muted),
              text("1-5 直接训练") | color(palette.muted),
              text("t 切换主题  b 切换背景") | color(palette.muted),
          }),
          palette);

      Element today_panel = Section(
          "今天建议",
          vbox({
              text(plan.headline.empty() ? "按数字直接开始即可。" : plan.headline) | color(palette.text),
              text("单词优先，伙伴只做提示与反馈。") | color(palette.muted),
          }),
          palette);

      Element companion_panel = Section("陪练伙伴", CompanionCard(companion, palette), palette);

      Element layout = vbox(Elements{
                           hbox(Elements{
                               text("WordRush") | bold | color(Color::RGB(248, 227, 179)),
                               filler(),
                               text(palette.name + "  ·  " +
                                        std::string(palette.use_background ? "主题背景开" : "终端背景开")) |
                                   color(palette.muted),
                           }),
                           separator(),
                           hbox(Elements{
                               books_panel | size(WIDTH, EQUAL, 24),
                               separator(),
                               vbox(Elements{
                                   hbox(Elements{
                                       current_panel | flex,
                                       separator(),
                                       status_panel | size(WIDTH, EQUAL, 22),
                                    }),
                                    separator(),
                                    shortcut_panel,
                                    separator(),
                                    today_panel,
                               }) | flex,
                               separator(),
                               vbox(Elements{
                                   companion_panel,
                                   separator(),
                                   action_panel,
                               }) | size(WIDTH, EQUAL, 24),
                           }) |
                               flex,
                           separator(),
                           hbox(Elements{
                               text("j / k 切换") | color(palette.muted),
                               filler(),
                               text("1 速刷  2 中译英  3 英译中  4 混译  5 错词") | color(palette.blue),
                               text("   m 管理") | color(palette.muted),
                               text("   t 主题") | color(palette.muted),
                               text("   b 背景") | color(palette.muted),
                               text("   ? 帮助") | color(palette.muted),
                               text("   q 退出") | color(palette.muted),
                            }),
                        }) |
                        color(palette.text);

      if (palette.use_background) {
        layout = layout | bgcolor(palette.background);
      }

      if (overlay == Overlay::Help) {
        Element modal = vbox({
                            filler(),
                            hbox(Elements{filler(), HelpModal(palette), filler()}),
                            filler(),
                        });
        if (palette.use_background) {
          modal = modal | bgcolor(palette.overlay);
        }
        return dbox({layout, modal});
      }

      if (overlay == Overlay::Plan) {
        Element modal = vbox({
                            filler(),
                            hbox(Elements{filler(), PlanModal(current, shortcuts, plan, palette), filler()}),
                            filler(),
                        });
        if (palette.use_background) {
          modal = modal | bgcolor(palette.overlay);
        }
        return dbox({layout, modal});
      }

      return layout;
    });

    auto app = CatchEvent(renderer, [&](Event event) {
      if (overlay != Overlay::None) {
        if (event == Event::Escape || event == Event::Character('q') || event == Event::Return ||
            event == Event::Character('?')) {
          overlay = Overlay::None;
          return true;
        }
        return true;
      }

      if (event == Event::Escape || event == Event::Character('q')) {
        screen.Exit();
        return true;
      }

      if (event == Event::Character('?')) {
        overlay = Overlay::Help;
        return true;
      }

      if (books_.empty()) {
        if (event == Event::Character('m')) {
          requested_manage = true;
          screen.Exit();
          return true;
        }
        return false;
      }

      if (event == Event::Character('j') || event == Event::ArrowDown) {
        selected = (selected + 1) % books_.size();
        return true;
      }

      if (event == Event::Character('k') || event == Event::ArrowUp) {
        selected = (selected + books_.size() - 1) % books_.size();
        return true;
      }

      if (event == Event::Character('m')) {
        requested_manage = true;
        screen.Exit();
        return true;
      }

      if (event == Event::Character('t')) {
        ui_settings.theme_index = (ui_settings.theme_index + 1) % BuiltinThemes().size();
        SaveUiSettings(project_root_, ui_settings);
        return true;
      }

      if (event == Event::Character('b')) {
        ui_settings.use_theme_background = !ui_settings.use_theme_background;
        SaveUiSettings(project_root_, ui_settings);
        return true;
      }

      if (event == Event::Return) {
        overlay = Overlay::Plan;
        return true;
      }

      if (event == Event::Character('1')) {
        requested_mode = DrillMode::Scan;
        requested_variant = RecallVariant::CnToEn;
        screen.Exit();
        return true;
      }

      if (event == Event::Character('2')) {
        requested_mode = DrillMode::TypeRecall;
        requested_variant = RecallVariant::CnToEn;
        screen.Exit();
        return true;
      }

      if (event == Event::Character('3')) {
        requested_mode = DrillMode::TypeRecall;
        requested_variant = RecallVariant::EnToCn;
        screen.Exit();
        return true;
      }

      if (event == Event::Character('4')) {
        requested_mode = DrillMode::TypeRecall;
        requested_variant = RecallVariant::Mixed;
        screen.Exit();
        return true;
      }

      if (event == Event::Character('5')) {
        requested_mode = DrillMode::MistakeBurst;
        requested_variant = RecallVariant::Mixed;
        screen.Exit();
        return true;
      }

      return false;
    });

    screen.Loop(app);

    if (requested_manage) {
      manage_books();
      continue;
    }

    if (!requested_mode.has_value()) {
      return 0;
    }

    WordBookRepository repository(project_root_);
    WordBookDetail detail = repository.LoadBookDetail(books_[selected].meta.id);
    detail.progress = books_[selected].progress;
    DrillSession session(std::move(detail),
                         *requested_mode,
                         requested_variant.value_or(RecallVariant::CnToEn),
                         ui_settings.theme_index,
                         ui_settings.use_theme_background);
    session.Run();
  }
}

}  // namespace wordrush
