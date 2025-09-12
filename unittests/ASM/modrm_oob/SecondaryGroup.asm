%ifdef CONFIG
{
  "RegData": {
    "RAX": "1"
  },
  "MemoryRegions": {
    "0x100000000": "4096",
    "0x100002000": "4096"
  }
}
%endif

mov r15, 0x100001000
mov r14, 0x100002000
mov rax, 0

%include "modrm_oob_macros.mac"

; Secondary Group
; w2 sldt, 2
; w2 str, 2
; w2 verr, 2
; w2 verw, 2
; SGDT
; SIDT
w2 smsw, 2

w4_size bt, 2, word, 0
w4_size bt, 4, dword, 0
w4_size bt, 8, qword, 0

w4_size bts, 2, word, 0
w4_size bts, 4, dword, 0
w4_size bts, 8, qword, 0

w4_size btr, 2, word, 0
w4_size btr, 4, dword, 0
w4_size btr, 8, qword, 0

w4_size btc, 2, word, 0
w4_size btc, 4, dword, 0
w4_size btc, 8, qword, 0

w2 cmpxchg8b, 8
w2 cmpxchg16b, 16

w2 fxsave, 512
r2 fxrstor, 512

w2 stmxcsr, 4
r2 ldmxcsr, 4

; XSAVE/XRSTOR size is variable and can't be tested here.

; Done
mov rax, 1
hlt
