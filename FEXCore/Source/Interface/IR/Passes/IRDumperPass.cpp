// SPDX-License-Identifier: MIT
/*
$info$
tags: ir|debug
desc: Prints IR
$end_info$
*/

#include "Interface/IR/IR.h"
#include "Interface/IR/IREmitter.h"
#include "Interface/IR/PassManager.h"
#include "Interface/IR/Passes/RegisterAllocationPass.h"
#include "Interface/Core/OpcodeDispatcher.h"

#include <FEXCore/IR/IR.h>

namespace FEXCore::IR::Debug {
class IRDumper final : public FEXCore::IR::Pass {
public:
  IRDumper();
  void Run(IREmitter* IREmit) override;

private:
  FEX_CONFIG_OPT(DumpIR, DUMPIR);
  bool DumpToFile {};
  bool DumpToLog {};
};

IRDumper::IRDumper() {
  const auto DumpIRStr = DumpIR();
  if (DumpIRStr == "stderr" || DumpIRStr == "stdout" || DumpIRStr == "no") {
    // Intentionally do nothing
  } else if (DumpIRStr == "server") {
    DumpToLog = true;
  } else {
    DumpToFile = true;
  }
}

void IRDumper::Run(IREmitter* IREmit) {
  auto RAPass = Manager->GetPass<IR::RegisterAllocationPass>("RA");
  IR::RegisterAllocationData* RA {};
  if (RAPass) {
    RA = RAPass->GetAllocationData();
  }

  FEXCore::File::File FD {};
  if (DumpIR() == "stderr") {
    FD = FEXCore::File::File::GetStdERR();
  } else if (DumpIR() == "stdout") {
    FD = FEXCore::File::File::GetStdOUT();
  }

  auto IR = IREmit->ViewIR();
  auto HeaderOp = IR.GetHeader();
  LOGMAN_THROW_AA_FMT(HeaderOp->Header.Op == OP_IRHEADER, "First op wasn't IRHeader");

  // DumpIRStr might be no if not dumping but ShouldDump is set in OpDisp
  if (DumpToFile) {
    const auto fileName = fextl::fmt::format("{}/{:x}{}", DumpIR(), HeaderOp->OriginalRIP, RA ? "-post.ir" : "-pre.ir");
    FD = FEXCore::File::File(fileName.c_str(),
                             FEXCore::File::FileModes::WRITE | FEXCore::File::FileModes::CREATE | FEXCore::File::FileModes::TRUNCATE);
  }

  if (FD.IsValid() || DumpToLog) {
    fextl::stringstream out;
    FEXCore::IR::Dump(&out, &IR, RA);
    if (FD.IsValid()) {
      fextl::fmt::print(FD, "IR-{} 0x{:x}:\n{}\n@@@@@\n", RA ? "post" : "pre", HeaderOp->OriginalRIP, out.str());
    } else {
      LogMan::Msg::IFmt("IR-{} 0x{:x}:\n{}\n@@@@@\n", RA ? "post" : "pre", HeaderOp->OriginalRIP, out.str());
    }
  }
}

fextl::unique_ptr<FEXCore::IR::Pass> CreateIRDumper() {
  return fextl::make_unique<IRDumper>();
}
} // namespace FEXCore::IR::Debug
