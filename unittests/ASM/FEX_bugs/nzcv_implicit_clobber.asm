%ifdef CONFIG
{
  "RegData": {
    "RAX": "0",
    "XMM0": ["0", "0"]
  }
}
%endif

; FEX has a bug with NZCV host flag usage that IR operations that implicitly clobber flags might not save emulated eflags correctly in all instances.
; This tests one particular instance of `ImplicitFlagClobber`.
movaps xmm0, [rel .data]

; Calculate ZF up-front
mov eax, 1
add eax, eax

; This jump is necessary to break visibility.
jmp .begin
.begin:

; minss turns in to VFMinScalarInsert which implicitly clobbers Arm64 flags.
; Potentially any instruction that uses an IR operation that uses `ImplicitFlagClobber` would break.
minss   xmm0, xmm0

; Ensure the flags calculated by the `add eax, eax` are consumed.
; ZF should be unset from `add 1, 1`.
; If minss clobbers Arm64 host flags then the `fcmp` that Arm64 uses will overwrite nzcv, thus setting the ZF flag.
; This is since `fcmp #0, #0` will set nzcv to `0110`.
jnz .next
mov eax, 1
hlt

.next:
mov eax, 0
hlt

align 16
.data:
dd 0, 0, 0, 0
