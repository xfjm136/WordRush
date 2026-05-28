#include "wordrush/companion.hpp"

#include <algorithm>
#include <array>

namespace wordrush {

namespace {

std::string MoodFor(double mastery) {
  if (mastery >= 0.8) {
    return "很有精神";
  }
  if (mastery >= 0.5) {
    return "认真陪练";
  }
  return "刚睡醒";
}

std::string SpeechFor(int affinity, int level, double mastery) {
  static const std::array<std::string, 8> lines = {
      "今天也一起狠狠干掉这些单词吧。",
      "我已经闻到进步的味道了。",
      "先别急，慢一点，记得更牢。",
      "你拼对的时候，我都替你开心。",
      "错词别怕，回头再捶一遍就行。",
      "背词不是打卡，是把脑子练顺。",
      "我会在你想放弃的时候推你一下。",
      "记得越多，我就越想给你讲个故事。"};

  const std::size_t index = static_cast<std::size_t>((affinity + level + static_cast<int>(mastery * 10.0)) %
                                                     static_cast<int>(lines.size()));
  return lines[index];
}

std::vector<std::string> ActionsFor(double mastery, int correct, int wrong) {
  if (mastery >= 0.8 && wrong <= 1) {
    return {"打滚", "转圈", "讲笑话"};
  }
  if (correct > wrong) {
    return {"点头", "挥手", "抖耳朵"};
  }
  return {"蹲下", "眨眼", "给你递咖啡"};
}

std::string AsciiArt(int level, double mastery) {
  if (mastery >= 0.85) {
    return " /\\_/\\\\\n( o.o )\n > ^ < ";
  }
  if (level >= 3) {
    return " /\\_/\\\\\n( o.o )\n /   \\";
  }
  return " /\\_/\\\\\n( -.- )\n /   \\";
}

}  // namespace

CompanionState BuildCompanionState(const std::vector<WordBookOverview>& books) {
  CompanionState state;
  state.name = "Lexi";

  std::size_t entries = 0;
  std::size_t mastered = 0;
  std::size_t due = 0;
  for (const auto& book : books) {
    entries += book.meta.total_entries;
    mastered += book.progress.mastered_items;
    due += book.progress.due_now;
  }

  const double mastery = entries == 0 ? 0.0 : static_cast<double>(mastered) / static_cast<double>(entries);
  state.level = std::max(1, static_cast<int>(mastered / 20) + 1);
  state.affinity = static_cast<int>(mastered + due * 2);
  state.mood = MoodFor(mastery);
  state.posture = mastery >= 0.75 ? "站得笔直" : "趴在词本上";
  state.ascii_art = AsciiArt(state.level, mastery);
  state.speech = SpeechFor(state.affinity, state.level, mastery);
  state.actions = ActionsFor(mastery, static_cast<int>(mastered), static_cast<int>(due));
  return state;
}

CompanionState BuildCompanionState(const WordBookDetail& book, int correct, int wrong, std::size_t index) {
  CompanionState state;
  state.name = "Lexi";
  const double mastery = book.meta.total_entries == 0
                             ? 0.0
                             : static_cast<double>(book.progress.mastered_items + static_cast<std::size_t>(correct)) /
                                   static_cast<double>(book.meta.total_entries);
  state.level = std::max(1, static_cast<int>((book.progress.mastered_items + correct) / 15) + 1);
  state.affinity = correct * 3 - wrong + static_cast<int>(index);
  state.mood = MoodFor(mastery);
  state.posture = wrong > correct ? "有点紧张" : "很起劲";
  state.ascii_art = AsciiArt(state.level, mastery);
  state.speech = SpeechFor(state.affinity, state.level, mastery);
  state.actions = ActionsFor(mastery, correct, wrong);
  return state;
}

}  // namespace wordrush
