#ifndef LLVM_TRANSFORMS_MYPASSES_LOOPPEELING_H
#define LLVM_TRANSFORMS_MYPASSES_LOOPPEELING_H

#include "llvm/IR/PassManager.h"
#include "llvm/Analysis/LoopInfo.h"

namespace llvm {

    class LoopPeelingPass : public PassInfoMixin<LoopPeelingPass> {
    public:
        PreservedAnalyses run(Loop &L, LoopAnalysisManager &AM,
                              LoopStandardAnalysisResults &AR, LPMUpdater &U);
    };

} // namespace llvm

#endif // LLVM_TRANSFORMS_MYPASSES_LOOPPEELING_H