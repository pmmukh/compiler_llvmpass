#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/Support/BlockFrequency.h"
#include "llvm/Analysis/BlockFrequencyInfo.h"
#include "llvm/Analysis/BranchProbabilityInfo.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/IR/Dominators.h"
#include "string.h"
#include "llvm/Analysis/MemoryLocation.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
using namespace llvm;

namespace {


	struct SkeletonPass : public BlockFrequencyInfoWrapperPass {
		static char ID;
		SkeletonPass() :BlockFrequencyInfoWrapperPass() {
			initializeBlockFrequencyInfoWrapperPassPass(*PassRegistry::getPassRegistry());
		}

		virtual bool runOnFunction(Function &F) {
			//F.dump();

			BranchProbabilityInfo &BPI =
				getAnalysis<BranchProbabilityInfoWrapperPass>().getBPI();
			LoopInfo &LI = getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
			getBFI().calculate(F, BPI, LI);
			int argcnt = 0;
			const char *fname = F.getName().data();
			Instruction* mallocinstr;
			if (strcmp(fname, "malloc_fast") && strcmp(fname, "malloc") && strcmp(fname, "malloc_nvm")) {
				//errs() << "Entering function : " << F.getName() << "!\n";
				std::vector<Value*> storeops;
				std::map<Instruction*, Value*> mallstmap;
				for (auto& B : F) {
					int findstore = 0;
					for (auto& I : B) {
						const char *opcode = I.getOpcodeName();
						if (CallInst* ci = dyn_cast<CallInst>(&I)) {
							Function *fun = ci->getCalledFunction();
							if (strcmp(fun->getName().data(), "malloc") == 0) {
								mallocinstr = &I;
								findstore = 1;
							}
						}
						if (findstore == 1 && strcmp(opcode, "store") == 0) {
							Value* stop = I.getOperand(1);
							errs() << "Data obj identified is " << stop << "\n";
							storeops.push_back(stop);
							mallstmap[mallocinstr] = stop;
							findstore = 0;
						}

					}
				}

				std::map<Value *, uint64_t> freqmap;
				std::map<Value *, uint64_t>::iterator it;
				for (auto& B : F) {
					BlockFrequency bf = getBFI().getBlockFreq(&B);
					for (auto& I : B) {
						const char *opcode = I.getOpcodeName();
						if (strcmp(opcode, "store") == 0 || strcmp(opcode, "load") == 0) {
							for (unsigned i = 0; i < I.getNumOperands(); i++) {
								Value* oper = I.getOperand(i);
								if (std::find(storeops.begin(), storeops.end(), oper) != storeops.end()) {
									errs() << "Matched instr is : ";
									it = freqmap.find(oper);
									if (it != freqmap.end()) {
										freqmap[oper] += bf.getFrequency();
									}
									else {
										freqmap[oper] = bf.getFrequency();
									}
									I.dump();
								}
							}
						}
					}
				}
				errs() << "Printing the elements\n";
				for (auto elem : freqmap)
				{
					errs() << elem.first << " " << elem.second << "\n";
				}

				Function *mlfast = F.getParent()->getFunction("malloc_fast");
				Function *mlnvm = F.getParent()->getFunction("malloc_nvm");
				std::map <CallInst *, CallInst *> instr_list;
				for (auto& B : F) {
					//errs() << "Basic block:\n";
					BlockFrequency bf = getBFI().getBlockFreq(&B);
					//errs() << "Freq: " << bf.getFrequency() << " \n";
					for (auto& I : B) {
						const char *opcode = I.getOpcodeName();
						if (CallInst* ci = dyn_cast<CallInst>(&I)) {
							Function *fun = ci->getCalledFunction();
							if (strcmp(fun->getName().data(), "malloc") == 0) {
								IRBuilder<> builder(&B);
								std::vector<Value*> args1;
								for (unsigned i = 0; i<ci->getNumArgOperands(); i++) {
									Value* x = ci->getArgOperand(i);
									args1.push_back(x);
								}
								//CallInst* recur_1 = builder.CreateCall(mlfast, llvm::makeArrayRef(args1), "mlfast");

								uint64_t freq = freqmap[mallstmap[&I]];
								CallInst* repl;
								if (freq < 200) {
									repl = CallInst::Create(mlnvm, llvm::makeArrayRef(args1));
								}
								else if (freq>1900) {
									repl = CallInst::Create(mlfast, llvm::makeArrayRef(args1));
								}
								//CallInst* repl = CallInst::Create(mlfast, llvm::makeArrayRef(args1));
								if(freq<200 || freq > 1900)
									instr_list[ci] = repl;

							}
						}
					}
				}
				errs() << "Instructions replaced: ";
				for (std::map<CallInst *, CallInst *>::iterator it = instr_list.begin(); it != instr_list.end(); ++it) {
					errs() << "Original: ";
					it->first->dump();
					errs() << "New: ";
					it->second->dump();
					ReplaceInstWithInst(it->first, it->second);
				}

				for (auto& B : F) {
					for (auto& I : B) {
						errs() << "Instruction: ";
						I.dump();
					}
				}
			}
			return false;
		}
	};

}

char BlockFrequencyInfoWrapperPass::ID = 0;
// Automatically enable the pass.
// http://adriansampson.net/blog/clangpass.html
static void registerSkeletonPass(const PassManagerBuilder &,
	legacy::PassManagerBase &PM) {
	PM.add(new SkeletonPass());
}
static RegisterStandardPasses
RegisterMyPass(PassManagerBuilder::EP_EarlyAsPossible,
	registerSkeletonPass);
