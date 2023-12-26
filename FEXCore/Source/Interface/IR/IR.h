// SPDX-License-Identifier: MIT
#pragma once

#include <FEXCore/Utils/ThreadPoolAllocator.h>
#include <FEXCore/IR/IR.h>

#include <FEXCore/fextl/memory.h>
#include <FEXCore/fextl/sstream.h>

namespace FEXCore::IR {

class RegisterAllocationData;

class IRListView;
class IREmitter;

void Dump(fextl::stringstream *out, IRListView const* IR, IR::RegisterAllocationData *RAData);
fextl::unique_ptr<IREmitter> Parse(FEXCore::Utils::IntrusivePooledAllocator &ThreadAllocator, fextl::stringstream &MapsStream);
}
