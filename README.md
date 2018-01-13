# compiler_llvmpass
Optimized Data Allocation 

This LLVM pass deals with how compiler optimization is performed in LLVM/Clang. The steps are as follows:

1. Identify all the data objects that have been allocated with malloc()
2. Analyse memory read and write frequency for the data objects, using BlockFrequencyInfoWrapper
3. Optimize the data allocation process by allocating high write latency objects with malloc_fast, low write latency objects with mallo_nvm and the rest with malloc
