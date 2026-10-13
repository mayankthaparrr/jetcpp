//this is toolchain.h
#ifndef ATR_TOOLCHAIN_H
#define ATR_TOOLCHAIN_H

void compileOnly(void);     // Run -> Compile
void compileAndRun(void);   // Run -> Run
void compileAndDebug(void); // Run -> Debug
void stopProgram(void);     // Run -> Stop
void toolchainUpdate(void); // every frame: pull in clang's output, run/debug once it's done
int findTool(const char *name, char *out, int size); // full path to clang.exe, lldb.exe, clangd.exe...

#endif
