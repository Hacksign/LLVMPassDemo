#include "llvm/IR/Function.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Plugins/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace {
struct SimpleObfuscationPass : PassInfoMixin<SimpleObfuscationPass> {
  static bool isRequired() { return true; }

  PreservedAnalyses run(Function &F, FunctionAnalysisManager &) {
    errs() << "[Obfuscation] 正在扫描函数: " << F.getName() << "\n";

    for (inst_iterator I = inst_begin(F), E = inst_end(F); I != E; ) {
      Instruction &Inst = *I;
      ++I; // 提前递增迭代器，防止删除指令后迭代器失效

      // 检查是否是整数类型的加法指令
      if (auto *BinOp = dyn_cast<BinaryOperator>(&Inst)) {
        if (BinOp->getOpcode() == Instruction::Add && BinOp->getType()->isIntegerTy()) {
          Value *LHS = BinOp->getOperand(0); // 获取左操作数 a
          Value *RHS = BinOp->getOperand(1); // 获取右操作数 b

          // 使用 IRBuilder 在原指令位置直接创建一条减法指令：a - b
          IRBuilder<> Builder(BinOp);
          Value *NewSub = Builder.CreateSub(LHS, RHS, "replaced_sub");

          // 将所有使用旧加法指令的地方，全部替换成这条新的减法指令
          BinOp->replaceAllUsesWith(NewSub);

          // 删除旧的加法指令
          BinOp->eraseFromParent();

          errs() << "  [混淆成功] 已将加法指令 (a + b) 直接替换为减法指令 (a - b)\n";
        }
      }
    }
    return PreservedAnalyses::none();
  }
};
} // namespace

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return {
      LLVM_PLUGIN_API_VERSION, "SimpleObfuscationPass", LLVM_VERSION_STRING,
      [](PassBuilder &PB) {
        PB.registerPipelineParsingCallback(
            [](StringRef Name, FunctionPassManager &FPM,
               ArrayRef<PassBuilder::PipelineElement>) {
              if (Name == "simple-obf") {
                FPM.addPass(SimpleObfuscationPass());
                return true;
              }
              return false;
            });
      }};
}
