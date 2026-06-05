#include "llvm/IR/Function.h"
#include "llvm/IR/InstIterator.h" // 修复报错1：引入 inst_iterator 所需的头文件
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IRBuilder.h"    // 修复报错2：引入 IRBuilder 用于标准地插入指令
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

    // 使用 inst_iterator 遍历，安全地处理边遍历边删除的情况
    for (inst_iterator I = inst_begin(F), E = inst_end(F); I != E; ) {
      Instruction &Inst = *I;
      ++I; // 提前递增迭代器，防止删除当前指令后迭代器失效

      if (auto *BinOp = dyn_cast<BinaryOperator>(&Inst)) {
        // 只混淆整数类型的加法指令
        if (BinOp->getOpcode() == Instruction::Add && BinOp->getType()->isIntegerTy()) {
          Value *LHS = BinOp->getOperand(0);
          Value *RHS = BinOp->getOperand(1);

          // 使用 IRBuilder 在旧指令的位置插入新指令，这是最标准、最安全的方式
          IRBuilder<> Builder(BinOp);

          // 1. 创建 -b (对右边的操作数取负)
          Value *NegRHS = Builder.CreateNeg(RHS, "neg_tmp");

          // 2. 创建 a - (-b) (用左边的操作数减去取负后的结果)
          Value *NewSub = Builder.CreateSub(LHS, NegRHS, "sub_tmp");

          // 3. 将原程序中所有使用旧加法指令的地方，全部替换成新创建的减法指令
          BinOp->replaceAllUsesWith(NewSub);

          // 4. 彻底删除旧的加法指令
          BinOp->eraseFromParent();

          errs() << "  [混淆成功] 已将一条加法指令替换为 a - (-b)\n";
        }
      }
    }
    // 因为我们修改了 IR 的结构，必须返回 none
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
