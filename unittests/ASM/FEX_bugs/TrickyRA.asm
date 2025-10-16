%ifdef CONFIG
{
  "RegData": {
  }
}
%endif

; This test is reduced from a game that hit a register allocation bug. The test
; has a high register pressure across a `rep movsb` (_Memcpy), and since _Memcpy
; is modelled as writing a GPRPair, this induces live range splitting at the
; time of writing. FEX had a bug where live range splitting was unsound in
; certain circumstances.

mov rsp, 0xe000_1000
lea rbp, [rel .data_mid]
lea rdi, [rel .data_dst]
lea rsi, [rel .data_src]
mov rcx, 11

; Store where it is expected
mov [rbp-0x9e8], rcx
mov rcx, rdi

jmp .test
.test:
mov     rax, qword [rbp-0x9f0]
mov     rdx, qword [rbp-0x9e8]
mov     rdi, rcx
mov     rcx, rdx
popfq
rep movsb ; Uses RDI and RSI
pushfq
mov     qword [rbp-0x9f0], rax
mov     qword [rbp-0x9e8], rdx

hlt

align 4096
.data:
times 4096 db 0
.data_mid:
times 4096 db 0

.data_dst:
times 16 db 0

.data_src:
times 16 db 0

