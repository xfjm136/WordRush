#pragma once

#include <vector>

#include "wordrush/training.hpp"
#include "wordrush/types.hpp"

namespace wordrush {

class DrillSession {
 public:
  DrillSession(WordBookDetail book, DrillMode mode, RecallVariant recall_variant = RecallVariant::CnToEn);

  int Run();

 private:
  WordBookDetail book_;
  DrillMode mode_;
  RecallVariant recall_variant_;
};

}  // namespace wordrush
