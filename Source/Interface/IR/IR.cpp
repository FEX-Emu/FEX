#include <FEXCore/IR/IR.h>
#include <FEXCore/IR/IntrusiveIRList.h>

namespace FEXCore::IR {
#define IROP_GETNAME_IMPL
#include "IRDefines.inc"

static void PrintArg(std::stringstream *out, [[maybe_unused]] IRListView<false> const* IR, uint64_t Arg) {
  *out << "0x" << std::hex << Arg;
}

static void PrintArg(std::stringstream *out, IRListView<false> const* IR, NodeWrapper Arg) {
  uintptr_t Data = IR->GetData();
  uintptr_t ListBegin = IR->GetListData();

  OrderedNode *RealNode = reinterpret_cast<OrderedNode*>(Arg.GetPtr(ListBegin));
  auto IROp = RealNode->Op(Data);

  *out << "%ssa" << std::to_string(Arg.NodeOffset / sizeof(OrderedNode)) << " i" << std::dec << (IROp->Size * 8);
  if (IROp->Elements > 1) {
    *out << "v" << std::dec << IROp->Elements;
  }
}

void Dump(std::stringstream *out, IRListView<false> const* IR) {
  uintptr_t Data = IR->GetData();
  uintptr_t ListBegin = IR->GetListData();

  auto Begin = IR->begin();
  auto End = IR->end();
  while (Begin != End) {
    auto Op = Begin();
    OrderedNode *RealNode = reinterpret_cast<OrderedNode*>(Op->GetPtr(ListBegin));
    auto IROp = RealNode->Op(Data);

		auto Name = FEXCore::IR::GetName(IROp->Op);

    if (IROp->HasDest) {
      *out << "%ssa" << std::to_string(Op->NodeOffset / sizeof(OrderedNode)) << " i" << std::dec << (IROp->Size * 8);
      if (IROp->Elements > 1) {
        *out << "v" << std::dec << IROp->Elements;
      }
      *out << " = ";
    }

    *out << Name;
    switch (IROp->Op) {
    case IR::OP_BEGINBLOCK:
      *out << " %ssa" << std::to_string(Op->ID());
      break;
    default: break;
    }

#define IROP_ARGPRINTER_HELPER
#include "IRDefines.inc"
    default: *out << "<Unknown Args>"; break;
    }

    *out << "\n";

    ++Begin;
  }

}

}
