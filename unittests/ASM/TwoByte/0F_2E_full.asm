%ifdef CONFIG
{
  "RegData": {
    "RBX": "0x4200",
    "RCX": "0x0300",
    "RDX": "0x0200",
    "RSI": "0x4700",
    "RDI": "0x4200",
    "R8":  "0x4200"
  }
}
%endif

; Exhaustive UCOMISS flag tests covering all 5 flag outcomes.
; Existing 0F_2E.asm only tests a<b and NaN.
;
; LAHF result in AH (bit 0=CF, bit 2=PF, bit 6=ZF, bit 1=always 1):
;   a == b:    CF=0, ZF=1, PF=0 -> AH=0x42 -> RAX=0x4200
;   a < b:     CF=1, ZF=0, PF=0 -> AH=0x03 -> RAX=0x0300
;   a > b:     CF=0, ZF=0, PF=0 -> AH=0x02 -> RAX=0x0200
;   unordered: CF=1, ZF=1, PF=1 -> AH=0x47 -> RAX=0x4700

mov rdx, 0xe0000000

mov eax, 0x3f800000   ; 1.0
mov [rdx + 0], eax
mov eax, 0x40800000   ; 4.0
mov [rdx + 4], eax
mov eax, 0x7fc00000   ; QNaN
mov [rdx + 8], eax
mov eax, 0x00000000   ; +0.0
mov [rdx + 12], eax
mov eax, 0x80000000   ; -0.0
mov [rdx + 16], eax
mov eax, 0x7f800000   ; +Inf
mov [rdx + 20], eax

; Test 1: a == b (4.0 vs 4.0)
movss xmm0, [rdx + 4]
ucomiss xmm0, [rdx + 4]
mov rax, 0
lahf
mov rbx, rax

; Test 2: a < b (1.0 vs 4.0)
movss xmm0, [rdx + 0]
ucomiss xmm0, [rdx + 4]
mov rax, 0
lahf
mov rcx, rax

; Test 3: a > b (4.0 vs 1.0)
movss xmm0, [rdx + 4]
ucomiss xmm0, [rdx + 0]
mov rax, 0
lahf
mov rdx, rax

; Test 4: unordered (1.0 vs NaN)
mov rax, 0xe0000000
movss xmm0, [rax + 0]
ucomiss xmm0, [rax + 8]
mov rsi, 0
lahf
mov rsi, rax

; Test 5: +0.0 == -0.0
mov rax, 0xe0000000
movss xmm0, [rax + 12]
ucomiss xmm0, [rax + 16]
mov rdi, 0
lahf
mov rdi, rax

; Test 6: +Inf == +Inf
mov rax, 0xe0000000
movss xmm0, [rax + 20]
ucomiss xmm0, [rax + 20]
mov r8, 0
lahf
mov r8, rax

hlt
