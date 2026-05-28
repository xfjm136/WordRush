#pragma once

#include <vector>

#include "wordrush/training.hpp"
#include "wordrush/types.hpp"

namespace wordrush {

class DrillSession {
 public:
  DrillSession(WordBookDetail book,
               DrillMode mode,
               RecallVariant recall_variant = RecallVariant::CnToEn,
               std::size_t theme_index = 0,
               bool use_theme_background = true);

  int Run();

 private:
  WordBookDetail book_;
  DrillMode mode_;
  RecallVariant recall_variant_;
  std::size_t theme_index_;
  bool use_theme_background_;
};

}  // namespace wordrush
