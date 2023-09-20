// SPDX-License-Identifier: MIT
/* System instruction emitters.
 *
 * This is mostly a mashup of various instruction types.
 * Nothing follows an explicit pattern since they are mostly different.
 */
public:
    // System with result
    // TODO: SYSL
    // System Instruction
    // TODO: AT
    // TODO: CFP
    // TODO: CPP
    void dc(FEXCore::ARMEmitter::DataCacheOperation DCOp, FEXCore::ARMEmitter::Register rt) {
      constexpr uint32_t Op = 0b1101'0101'0000'1000'0111 << 12;
      SystemInstruction(Op, 0, FEXCore::ToUnderlying(DCOp), rt);
    }
    // TODO: DVP
    // TODO: IC
    // TODO: TLBI

    // Exception generation
    void svc(uint32_t Imm) {
      ExceptionGeneration(0b000, 0b000, 0b01, Imm);
    }
    void hvc(uint32_t Imm) {
      ExceptionGeneration(0b000, 0b000, 0b10, Imm);
    }
    void smc(uint32_t Imm) {
      ExceptionGeneration(0b000, 0b000, 0b11, Imm);
    }
    void brk(uint32_t Imm) {
      ExceptionGeneration(0b001, 0b000, 0b00, Imm);
    }
    void hlt(uint32_t Imm) {
      ExceptionGeneration(0b010, 0b000, 0b00, Imm);
    }
    void tcancel(uint32_t Imm) {
      ExceptionGeneration(0b011, 0b000, 0b00, Imm);
    }
    void dcps1(uint32_t Imm) {
      ExceptionGeneration(0b101, 0b000, 0b01, Imm);
    }
    void dcps2(uint32_t Imm) {
      ExceptionGeneration(0b101, 0b000, 0b10, Imm);
    }
    void dcps3(uint32_t Imm) {
      ExceptionGeneration(0b101, 0b000, 0b11, Imm);
    }
    // System instructions with register argument
    void wfet(FEXCore::ARMEmitter::Register rt) {
      SystemInstructionWithReg(0b0000, 0b000, rt);
    }
    void wfit(FEXCore::ARMEmitter::Register rt) {
      SystemInstructionWithReg(0b0000, 0b001, rt);
    }

    // Hints
    void nop() {
      Hint(FEXCore::ARMEmitter::HintRegister::NOP);
    }
    void yield() {
      Hint(FEXCore::ARMEmitter::HintRegister::YIELD);
    }
    void wfe() {
      Hint(FEXCore::ARMEmitter::HintRegister::WFE);
    }
    void wfi() {
      Hint(FEXCore::ARMEmitter::HintRegister::WFI);
    }
    void sev() {
      Hint(FEXCore::ARMEmitter::HintRegister::SEV);
    }
    void sevl() {
      Hint(FEXCore::ARMEmitter::HintRegister::SEVL);
    }
    void dgh() {
      Hint(FEXCore::ARMEmitter::HintRegister::DGH);
    }
    void csdb() {
      Hint(FEXCore::ARMEmitter::HintRegister::CSDB);
    }

    // Barriers
    void clrex(uint32_t imm = 15) {
      LOGMAN_THROW_AA_FMT(imm < 16, "Immediate out of range");
      Barrier(FEXCore::ARMEmitter::BarrierRegister::CLREX, imm);
    }
    void dsb(FEXCore::ARMEmitter::BarrierScope Scope) {
      Barrier(FEXCore::ARMEmitter::BarrierRegister::DSB, FEXCore::ToUnderlying(Scope));
    }
    void dmb(FEXCore::ARMEmitter::BarrierScope Scope) {
      Barrier(FEXCore::ARMEmitter::BarrierRegister::DMB, FEXCore::ToUnderlying(Scope));
    }
    void isb() {
      Barrier(FEXCore::ARMEmitter::BarrierRegister::ISB, FEXCore::ToUnderlying(FEXCore::ARMEmitter::BarrierScope::SY));
    }
    void sb() {
      Barrier(FEXCore::ARMEmitter::BarrierRegister::SB, 0);
    }
    void tcommit() {
      Barrier(FEXCore::ARMEmitter::BarrierRegister::TCOMMIT, 0);
    }

    // System register move
    void msr(FEXCore::ARMEmitter::SystemRegister reg, FEXCore::ARMEmitter::Register rt) {
      constexpr uint32_t Op = 0b1101'0101'0001 << 20;
      SystemRegisterMove(Op, rt, reg);
    }

    void mrs(FEXCore::ARMEmitter::Register rd, FEXCore::ARMEmitter::SystemRegister reg) {
      constexpr uint32_t Op = 0b1101'0101'0011 << 20;
      SystemRegisterMove(Op, rd, reg);
    }

private:

    // Exception Generation
    void ExceptionGeneration(uint32_t opc, uint32_t op2, uint32_t LL, uint32_t Imm) {
      LOGMAN_THROW_AA_FMT((Imm & 0xFFFF'0000) == 0, "Imm amount too large");

      uint32_t Instr = 0b1101'0100 << 24;

      Instr |= opc << 21;
      Instr |= Imm << 5;
      Instr |= op2 << 2;
      Instr |= LL;

      dc32(Instr);
    }

    // System instructions with register argument
    void SystemInstructionWithReg(uint32_t CRm, uint32_t op2, FEXCore::ARMEmitter::Register rt) {
      uint32_t Instr = 0b1101'0101'0000'0011'0001 << 12;

      Instr |= CRm << 8;
      Instr |= op2 << 5;
      Instr |= Encode_rt(rt);
      dc32(Instr);
    }

    // Hints
    void Hint(FEXCore::ARMEmitter::HintRegister Reg) {
      uint32_t Instr = 0b1101'0101'0000'0011'0010'0000'0001'1111U;
      Instr |= FEXCore::ToUnderlying(Reg);
      dc32(Instr);
    }
    // Barriers
    void Barrier(FEXCore::ARMEmitter::BarrierRegister Reg, uint32_t CRm) {
      uint32_t Instr = 0b1101'0101'0000'0011'0011'0000'0001'1111U;
      Instr |= CRm << 8;
      Instr |= FEXCore::ToUnderlying(Reg);
      dc32(Instr);
    }

    // System Instruction
    void SystemInstruction(uint32_t Op, uint32_t L, uint32_t SubOp, FEXCore::ARMEmitter::Register rt) {
      uint32_t Instr = Op;

      Instr |= L << 21;
      Instr |= SubOp;
      Instr |= Encode_rt(rt);

      dc32(Instr);
    }

    // System register move
    void SystemRegisterMove(uint32_t Op, FEXCore::ARMEmitter::Register rt, FEXCore::ARMEmitter::SystemRegister reg) {
      uint32_t Instr = Op;

      Instr |= FEXCore::ToUnderlying(reg);
      Instr |= Encode_rt(rt);

      dc32(Instr);
    }

