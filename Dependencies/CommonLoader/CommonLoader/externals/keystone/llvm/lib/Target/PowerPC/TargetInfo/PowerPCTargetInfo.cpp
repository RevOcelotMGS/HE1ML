//===-- PowerPCTargetInfo.cpp - PowerPC Target Implementation -------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "PPC.h"
#include "llvm/Support/TargetRegistry.h"
using namespace llvm;

Target llvm::ThePPC32Target, llvm::ThePPC64Target, llvm::ThePPC64LETarget;

extern "C" void LLVMInitializePowerPCTargetInfo() {
  RegisterTarget<Triple::ppc>
    X(ThePPC32Target, "ppc32", "PowerPC 32");

  RegisterTarget<Triple::ppc64>
    Y(ThePPC64Target, "ppc64", "PowerPC 64");

  RegisterTarget<Triple::ppc64le>
    Z(ThePPC64LETarget, "ppc64le", "PowerPC 64 LE");
}
