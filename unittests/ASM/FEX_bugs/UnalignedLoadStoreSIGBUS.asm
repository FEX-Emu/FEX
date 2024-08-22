%ifdef CONFIG
{
  "Env": {
    "FEX_TSOENABLED": "1",
    "FEX_TSOAUTOMIGATRION": "0"
  }
}
%endif

; FEX-Emu had a bug where SIGBUS handling of unaligned loadstores using FEAT_LRCPC would accidentally try using the FEAT_LSE atomic memory operation
; handlers. It wouldn't find the handler for FEAT_LRCPC instructions (because it was only supposed to handle FEAT_LSE instructions) and fault out.
; This happens because FEAT_LRCPC and FEAT_LSE instructions partially share an instruction encoding and FEX forgot to check for FEAT_LRCPC first
; before using the FEAT_LSE handler.
mov r15, 0xe000_0000

; Atomic unaligned load across 16-byte and 64-byte granule
mov rax, qword [r15 + 15]
mov rbx, qword [r15 + 63]

; Atomic unaligned store across 16-byte and 64-byte granule
mov qword [r15 + 15], rbx
mov qword [r15 + 63], rcx

hlt
