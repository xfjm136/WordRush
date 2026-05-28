#include "wordrush/training.hpp"

#include <algorithm>

namespace wordrush {

std::string ToString(DrillMode mode) {
  switch (mode) {
    case DrillMode::Scan:
      return "速刷";
    case DrillMode::TypeRecall:
      return "拼写";
    case DrillMode::MistakeBurst:
      return "错词";
  }

  return "拼写";
}

std::string ToString(RecallVariant variant) {
  switch (variant) {
    case RecallVariant::CnToEn:
      return "中译英";
    case RecallVariant::EnToCn:
      return "英译中";
    case RecallVariant::Mixed:
      return "混译";
  }

  return "中译英";
}

SessionRecommendation TrainingPlanner::BuildPlan(const WordBookOverview& book) const {
  SessionRecommendation plan;

  const std::size_t total = std::max<std::size_t>(book.meta.total_entries, 1);
  const bool review_heavy = book.progress.due_now >= 12 || book.progress.review_items >= 8;
  const bool accuracy_soft = book.progress.accuracy < 0.82;

  plan.new_cards_cap = static_cast<int>(std::clamp<std::size_t>(total / 5, 20, 80));
  plan.review_cards_cap = static_cast<int>(
      std::clamp<std::size_t>(book.progress.due_now + book.progress.learning_items + 10, 30, 140));

  if (review_heavy) {
    plan.headline = "今天复习压力偏高，先把到期词条清掉。";
    plan.rationale = "优先做主动回忆和错词回捶，别一开始就塞太多新词。";
  } else {
    plan.headline = "先速刷，再做拼写回忆，最后补错词。";
    plan.rationale = "应试冲刺里，键入输出最容易暴露真正没记住的词。";
  }

  if (review_heavy) {
    plan.blocks.push_back({
        .mode = DrillMode::TypeRecall,
        .recall_variant = RecallVariant::CnToEn,
        .label = "回忆冲刺",
        .reason = "根据提示快速回忆并键入英文，先把今天到期的内容处理掉。",
        .target_items = std::max(20, static_cast<int>(book.progress.due_now)),
        .target_minutes = 18,
    });
  } else {
    plan.blocks.push_back({
        .mode = DrillMode::Scan,
        .recall_variant = RecallVariant::CnToEn,
        .label = "速刷",
        .reason = "先快速扫一遍新词，后面的拼写回忆会更顺手，不会太生。", 
        .target_items = std::min(plan.new_cards_cap, 40),
        .target_minutes = 8,
    });
    plan.blocks.push_back({
        .mode = DrillMode::TypeRecall,
        .recall_variant = RecallVariant::CnToEn,
        .label = "拼写回忆",
        .reason = "根据中文释义或提示键入英文，强迫自己给出准确拼写。",
        .target_items = std::min(plan.review_cards_cap, 36),
        .target_minutes = 18,
    });
  }

  if (accuracy_soft || book.progress.due_now > 6) {
    plan.blocks.push_back({
        .mode = DrillMode::MistakeBurst,
        .recall_variant = RecallVariant::Mixed,
        .label = "错词回捶",
        .reason = "把刚错的词在几张之后立刻插回，比拖到明天更有效。",
        .target_items = 12,
        .target_minutes = 8,
    });
  }

  return plan;
}

}  // namespace wordrush
