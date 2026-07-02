#include "llvm/IR/Instruction.h"
#include "llvm/Transforms/Utils/LoopUtils.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Pass.h"
#include "llvm/Analysis/LoopPass.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace {
struct LoopUnswitchingPass : public LoopPass {

    std::vector<BasicBlock *> LoopBasicBlocks;
    std::unordered_map<Value *, Value *> VariablesMap;

    static char ID;
    LoopUnswitchingPass() : LoopPass(ID) {
    }

  void mapVariables(Loop *L) {

    for (BasicBlock &BB : *L->getLoopPreheader()->getParent()) {
      for (Instruction &I : BB) {
        if (isa<LoadInst>(&I))
          VariablesMap[&I] = I.getOperand(0);
      }
    }

  }

  BasicBlock *findCompareBlock() {

    for (size_t i = 1; i < LoopBasicBlocks.size(); i++) {
      for (Instruction &I : *LoopBasicBlocks[i]) {
        if (isa<ICmpInst>(&I))
          return LoopBasicBlocks[i];
      }
    }

    return nullptr;

  }

  bool canUnswitch() {

    Value *Var1 = nullptr;
    Value *Var2 = nullptr;
    
    BasicBlock *CompareBlock = findCompareBlock();
    if (CompareBlock == nullptr)
      return false;

    for (Instruction &I : *CompareBlock) {
      if (isa<ICmpInst>(&I)) {
        Var1 = VariablesMap[I.getOperand(0)];
        Var2 = VariablesMap[I.getOperand(1)];
        break;
      }
    }

    for (BasicBlock *BB : LoopBasicBlocks) {
      for (Instruction &I : *BB) {
        if (isa<StoreInst>(&I)) {
          if (I.getOperand(1) == Var1 || I.getOperand(1) == Var2)
            return false;
        }
      }
    }

    return true;

  }

  BasicBlock *copyBasicBlock(BasicBlock *OriginalBlock) {

    Instruction *Clone;
    std::unordered_map<Value *, Value *> Mapping;
    IRBuilder<> Builder(OriginalBlock->getContext());

    BasicBlock *NewBasicBlock = BasicBlock::Create(OriginalBlock->getContext(), "", OriginalBlock->getParent(), LoopBasicBlocks.front());

    Builder.SetInsertPoint(NewBasicBlock);

    for (Instruction &I : *OriginalBlock) {
      Clone = I.clone();
      Builder.Insert(Clone);
      Mapping[&I] = Clone;

      for (size_t i = 0; i < Clone->getNumOperands(); i++) {
        if (Mapping.find(Clone->getOperand(i)) != Mapping.end())
          Clone->setOperand(i, Mapping[Clone->getOperand(i)]);
      }
    }

    return NewBasicBlock;

  }

  BasicBlock *copyBlock(Loop *L) {

    BasicBlock *Exit = L->getExitBlock();
    std::unordered_map<Value *, Value *> Mapping;
    std::unordered_map<BasicBlock *, BasicBlock *> BlocksMapping;
    Instruction *Clone;
    IRBuilder<> Builder(Exit);
    BasicBlock *NewBasicBlock;

    for (BasicBlock *BB : LoopBasicBlocks) {
      NewBasicBlock = BasicBlock::Create(BB->getContext(), "", BB->getParent(), Exit);
      BlocksMapping[BB] = NewBasicBlock;
    }

    for (BasicBlock *BB : LoopBasicBlocks) {
      BasicBlock *Copy = BlocksMapping[BB];
      Builder.SetInsertPoint(Copy);

      for (Instruction &I : *BB) {
        Clone = I.clone();
        Mapping[&I] = Clone;
        Builder.Insert(Clone);

        for (size_t i = 0; i < Clone->getNumOperands(); i++) {
          if (Mapping.find(Clone->getOperand(i)) != Mapping.end())
            Clone->setOperand(i, Mapping[Clone->getOperand(i)]);
        }
      }
    }

    for (BasicBlock *BB : LoopBasicBlocks) {
      BasicBlock *Copy = BlocksMapping[BB];
      for (size_t i = 0; i < Copy->getTerminator()->getNumSuccessors(); i++) {
        if (BlocksMapping.find(Copy->getTerminator()->getSuccessor(i)) != BlocksMapping.end()) {
          Copy->getTerminator()->setSuccessor(i, BlocksMapping[Copy->getTerminator()->getSuccessor(i)]);
        }
      }
    }

    BasicBlock *CompareBlockCopy = BlocksMapping[findCompareBlock()];
    BasicBlock *LoopHeaderCopy = BlocksMapping[LoopBasicBlocks.front()];
    LoopHeaderCopy->getTerminator()->setSuccessor(0, CompareBlockCopy->getTerminator()->getSuccessor(1));

    CompareBlockCopy->getTerminator()->getSuccessor(0)->eraseFromParent();
    CompareBlockCopy->eraseFromParent();

    return BlocksMapping[LoopBasicBlocks.front()];

  }

  void unswitch(Loop *L) {

    BasicBlock *CompareBlock = findCompareBlock();
    BasicBlock *CopyBlock = copyBasicBlock(CompareBlock);

    L->getLoopPreheader()->getTerminator()->setSuccessor(0, CopyBlock);
    LoopBasicBlocks.front()->getTerminator()->setSuccessor(0, CopyBlock->getTerminator()->getSuccessor(0));

    BasicBlock *LoopCopy = copyBlock(L);
    CopyBlock->getTerminator()->setSuccessor(0, LoopBasicBlocks.front());
    CopyBlock->getTerminator()->setSuccessor(1, LoopCopy);

    CompareBlock->getTerminator()->getSuccessor(1)->eraseFromParent();
    CompareBlock->eraseFromParent();

  }

  bool runOnLoop(Loop *L, LPPassManager &LPM) override {
    mapVariables(L);
    LoopBasicBlocks = L->getBlocks();
    if (canUnswitch()) {
      unswitch(L);
      errs() << "Uspesan LoopUnswitching\n";
    }
    else
      errs() << "Neuspesan LoopUnswitching\n";


    return true;

  }

};

}

char LoopUnswitchingPass::ID = 0;
static RegisterPass<LoopUnswitchingPass> X("loop-unswitching", "Loop Unswitching Pass");
