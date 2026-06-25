#include "llvm/Transforms/Utils/LoopPeeling.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Support/raw_ostream.h"

#include "LoopPeeling.h"

using namespace llvm;

PreservedAnalyses LoopPeelingPass::run(Loop &L, LoopAnalysisManager &AM,
                                      LoopStandardAnalysisResults &AR, LPMUpdater &U) {


    return PreservedAnalyses::all();
}