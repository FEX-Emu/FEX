#include "TestDisassembler.h"

#include <catch2/catch_test_macros.hpp>
#include <fcntl.h>

using namespace ARMEmitter;

TEST_CASE_METHOD(TestDisassembler, "Emitter: System: System with result") {
  // TODO: Implement in emitter.
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: System: System Instruction") {
  // TODO: AT
  // TODO: CFP
  // TODO: CPP
  // vixl doesn't understand a bunch of data cache operation names.
  TEST_SINGLE(dc(DataCacheOperation::IVAC, Reg::r30), "sys #0, C7, C6, #1, x30");
  TEST_SINGLE(dc(DataCacheOperation::ISW, Reg::r30), "sys #0, C7, C6, #2, x30");
  TEST_SINGLE(dc(DataCacheOperation::CSW, Reg::r30), "sys #0, C7, C10, #2, x30");
  TEST_SINGLE(dc(DataCacheOperation::CISW, Reg::r30), "sys #0, C7, C14, #2, x30");
  TEST_SINGLE(dc(DataCacheOperation::ZVA, Reg::r30), "dc zva, x30");
  TEST_SINGLE(dc(DataCacheOperation::CVAC, Reg::r30), "dc cvac, x30");
  TEST_SINGLE(dc(DataCacheOperation::CVAU, Reg::r30), "dc cvau, x30");
  TEST_SINGLE(dc(DataCacheOperation::CIVAC, Reg::r30), "dc civac, x30");

  TEST_SINGLE(dc(DataCacheOperation::IGVAC, Reg::r30), "sys #0, C7, C6, #3, x30");
  TEST_SINGLE(dc(DataCacheOperation::IGSW, Reg::r30), "sys #0, C7, C6, #4, x30");
  TEST_SINGLE(dc(DataCacheOperation::IGDVAC, Reg::r30), "sys #0, C7, C6, #5, x30");
  TEST_SINGLE(dc(DataCacheOperation::IGDSW, Reg::r30), "sys #0, C7, C6, #6, x30");
  TEST_SINGLE(dc(DataCacheOperation::CGSW, Reg::r30), "sys #0, C7, C10, #4, x30");
  TEST_SINGLE(dc(DataCacheOperation::CGDSW, Reg::r30), "sys #0, C7, C10, #6, x30");
  TEST_SINGLE(dc(DataCacheOperation::CIGSW, Reg::r30), "sys #0, C7, C14, #4, x30");
  TEST_SINGLE(dc(DataCacheOperation::CIGDSW, Reg::r30), "sys #0, C7, C14, #6, x30");

  TEST_SINGLE(dc(DataCacheOperation::GVA, Reg::r30), "dc gva, x30");
  TEST_SINGLE(dc(DataCacheOperation::GZVA, Reg::r30), "dc gzva, x30");
  TEST_SINGLE(dc(DataCacheOperation::CGVAC, Reg::r30), "dc cgvac, x30");
  TEST_SINGLE(dc(DataCacheOperation::CGDVAC, Reg::r30), "dc cgdvac, x30");
  TEST_SINGLE(dc(DataCacheOperation::CGVAP, Reg::r30), "dc cgvap, x30");
  TEST_SINGLE(dc(DataCacheOperation::CGDVAP, Reg::r30), "dc cgdvap, x30");
  TEST_SINGLE(dc(DataCacheOperation::CGVADP, Reg::r30), "sys #3, C7, C13, #3, x30");
  TEST_SINGLE(dc(DataCacheOperation::CGDVADP, Reg::r30), "sys #3, C7, C13, #5, x30");
  TEST_SINGLE(dc(DataCacheOperation::CIGVAC, Reg::r30), "dc cigvac, x30");
  TEST_SINGLE(dc(DataCacheOperation::CIGDVAC, Reg::r30), "dc cigdvac, x30");

  TEST_SINGLE(dc(DataCacheOperation::CVAP, Reg::r30), "dc cvap, x30");

  TEST_SINGLE(dc(DataCacheOperation::CVADP, Reg::r30), "dc cvadp, x30");

  // TODO: DVP
  // TODO: IC
  // TODO: TLBI
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: System: Exception generation") {
  TEST_SINGLE(svc(65535), "svc #0xffff");
  TEST_SINGLE(hvc(65535), "hvc #0xffff");
  TEST_SINGLE(smc(65535), "smc #0xffff");
  TEST_SINGLE(brk(65535), "brk #0xffff");
  TEST_SINGLE(hlt(65535), "hlt #0xffff");
  TEST_SINGLE(tcancel(65535), "unimplemented (Unimplemented)");
  TEST_SINGLE(dcps1(65535), "dcps1 {#0xffff}");
  TEST_SINGLE(dcps2(65535), "dcps2 {#0xffff}");
  TEST_SINGLE(dcps3(65535), "dcps3 {#0xffff}");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: System: System instructions with register argument") {
  if (false) {
    // Unsupported in vixl.
    TEST_SINGLE(wfet(Reg::r30), "wfet x30");
    TEST_SINGLE(wfit(Reg::r30), "wfit x30");
  }
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: System: Hints") {
  TEST_SINGLE(nop(), "nop");
  TEST_SINGLE(yield(), "yield");
  TEST_SINGLE(wfe(), "wfe");
  TEST_SINGLE(wfi(), "wfi");
  TEST_SINGLE(sev(), "sev");
  TEST_SINGLE(sevl(), "sevl");
  TEST_SINGLE(dgh(), "dgh");
  TEST_SINGLE(csdb(), "csdb");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: System: Barriers") {
  TEST_SINGLE(clrex(0), "clrex #0x0");
  TEST_SINGLE(clrex(15), "clrex");

  TEST_SINGLE(dsb(BarrierScope::OSHLD), "dsb oshld");
  TEST_SINGLE(dsb(BarrierScope::OSHST), "dsb oshst");
  TEST_SINGLE(dsb(BarrierScope::OSH), "dsb osh");
  TEST_SINGLE(dsb(BarrierScope::NSHLD), "dsb nshld");
  TEST_SINGLE(dsb(BarrierScope::NSHST), "dsb nshst");
  TEST_SINGLE(dsb(BarrierScope::NSH), "dsb nsh");
  TEST_SINGLE(dsb(BarrierScope::ISHLD), "dsb ishld");
  TEST_SINGLE(dsb(BarrierScope::ISHST), "dsb ishst");
  TEST_SINGLE(dsb(BarrierScope::ISH), "dsb ish");
  TEST_SINGLE(dsb(BarrierScope::LD), "dsb ld");
  TEST_SINGLE(dsb(BarrierScope::ST), "dsb st");
  TEST_SINGLE(dsb(BarrierScope::SY), "dsb sy");

  TEST_SINGLE(dmb(BarrierScope::OSHLD), "dmb oshld");
  TEST_SINGLE(dmb(BarrierScope::OSHST), "dmb oshst");
  TEST_SINGLE(dmb(BarrierScope::OSH), "dmb osh");
  TEST_SINGLE(dmb(BarrierScope::NSHLD), "dmb nshld");
  TEST_SINGLE(dmb(BarrierScope::NSHST), "dmb nshst");
  TEST_SINGLE(dmb(BarrierScope::NSH), "dmb nsh");
  TEST_SINGLE(dmb(BarrierScope::ISHLD), "dmb ishld");
  TEST_SINGLE(dmb(BarrierScope::ISHST), "dmb ishst");
  TEST_SINGLE(dmb(BarrierScope::ISH), "dmb ish");
  TEST_SINGLE(dmb(BarrierScope::LD), "dmb ld");
  TEST_SINGLE(dmb(BarrierScope::ST), "dmb st");
  TEST_SINGLE(dmb(BarrierScope::SY), "dmb sy");

  TEST_SINGLE(isb(), "isb");

  TEST_SINGLE(sb(), "sb");
  TEST_SINGLE(tcommit(), "tcommit");
}
TEST_CASE_METHOD(TestDisassembler, "Emitter: System: System register move") {
  // vixl doesn't have decoding for a bunch of these.
  // Also most of these aren't writeable from el0, just testing the encoding.
  TEST_SINGLE(msr(SystemRegister::CTR_EL0, Reg::r30), "msr S3_3_c0_c0_1, x30");
  TEST_SINGLE(msr(SystemRegister::DCZID_EL0, Reg::r30), "msr dczid_el0, x30");
  TEST_SINGLE(msr(SystemRegister::TPIDR_EL0, Reg::r30), "msr S3_3_c13_c0_2, x30");
  TEST_SINGLE(msr(SystemRegister::RNDR, Reg::r30), "msr rndr, x30");
  TEST_SINGLE(msr(SystemRegister::RNDRRS, Reg::r30), "msr rndrrs, x30");
  TEST_SINGLE(msr(SystemRegister::NZCV, Reg::r30), "msr nzcv, x30");
  TEST_SINGLE(msr(SystemRegister::FPCR, Reg::r30), "msr fpcr, x30");
  TEST_SINGLE(msr(SystemRegister::TPIDRRO_EL0, Reg::r30), "msr S3_3_c13_c0_3, x30");
  TEST_SINGLE(msr(SystemRegister::CNTFRQ_EL0, Reg::r30), "msr S3_3_c14_c0_0, x30");
  TEST_SINGLE(msr(SystemRegister::CNTVCT_EL0, Reg::r30), "msr S3_3_c14_c0_2, x30");

  TEST_SINGLE(mrs(Reg::r30, SystemRegister::CTR_EL0), "mrs x30, S3_3_c0_c0_1");
  TEST_SINGLE(mrs(Reg::r30, SystemRegister::DCZID_EL0), "mrs x30, dczid_el0");
  TEST_SINGLE(mrs(Reg::r30, SystemRegister::TPIDR_EL0), "mrs x30, S3_3_c13_c0_2");
  TEST_SINGLE(mrs(Reg::r30, SystemRegister::RNDR), "mrs x30, rndr");
  TEST_SINGLE(mrs(Reg::r30, SystemRegister::RNDRRS), "mrs x30, rndrrs");
  TEST_SINGLE(mrs(Reg::r30, SystemRegister::NZCV), "mrs x30, nzcv");
  TEST_SINGLE(mrs(Reg::r30, SystemRegister::FPCR), "mrs x30, fpcr");
  TEST_SINGLE(mrs(Reg::r30, SystemRegister::TPIDRRO_EL0), "mrs x30, S3_3_c13_c0_3");
  TEST_SINGLE(mrs(Reg::r30, SystemRegister::CNTFRQ_EL0), "mrs x30, S3_3_c14_c0_0");
  TEST_SINGLE(mrs(Reg::r30, SystemRegister::CNTVCT_EL0), "mrs x30, S3_3_c14_c0_2");
}
