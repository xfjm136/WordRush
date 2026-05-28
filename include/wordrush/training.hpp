#pragma once

#include <string>
#include <vector>

#include "wordrush/types.hpp"

namespace wordrush {

enum class DrillMode {
  Scan,
  TypeRecall,
  MistakeBurst,
};

enum class RecallVariant {
  CnToEn,
  EnToCn,
  Mixed,
};

std::string ToString(DrillMode mode);
std::string ToString(RecallVariant variant);

struct DrillBlock {
  DrillMode mode = DrillMode::TypeRecall;
  RecallVariant recall_variant = RecallVariant::CnToEn;
  std::string label;
  std::string reason;
  int target_items = 0;
  int target_minutes = 0;
};

struct SessionRecommendation {
  std::string headline;
  std::string rationale;
  int new_cards_cap = 0;
  int review_cards_cap = 0;
  std::vector<DrillBlock> blocks;
};

class TrainingPlanner {
 public:
  SessionRecommendation BuildPlan(const WordBookOverview& book) const;
};

}  // namespace wordrush
