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

; Exhaustive UCOMISD flag tests covering all 5 flag outcomes.
; Tests double-precision variant of UCOMISS.
;
; LAHF result in AH:
;   a == b:    0x42, a < b: 0x03, a > b: 0x02, unordered: 0x47

mov rdx, 0xe0000000

mov rax, 0x3FF0000000000000   ; 1.0
mov [rdx + 0], rax
mov rax, 0x4010000000000000   ; 4.0
mov [rdx + 8], rax
mov rax, 0x7FF8000000000000   ; QNaN
mov [rdx + 16], rax
mov rax, 0x0000000000000000   ; +0.0
mov [rdx + 24], rax
mov rax, 0x8000000000000000   ; -0.0
mov [rdx + 32], rax
mov rax, 0x7FF0000000000000   ; +Inf
mov [rdx + 40], rax

; Test 1: a == b (4.0 vs 4.0)
movsd xmm0, [rdx + 8]
ucomisd xmm0, [rdx + 8]
mov rax, 0
lahf
mov rbx, rax

; Test 2: a < b (1.0 vs 4.0)
movsd xmm0, [rdx + 0]
ucomisd xmm0, [rdx + 8]
mov rax, 0
lahf
mov rcx, rax

; Test 3: a > b (4.0 vs 1.0)
movsd xmm0, [rdx + 8]
ucomisd xmm0, [rdx + 0]
mov rax, 0
lahf
mov rdx, rax

; Test 4: unordered (1.0 vs NaN)
mov rax, 0xe0000000
movsd xmm0, [rax + 0]
ucomisd xmm0, [rax + 16]
mov rsi, 0
lahf
mov rsi, rax

; Test 5: +0.0 == -0.0
mov rax, 0xe0000000
movsd xmm0, [rax + 24]
ucomisd xmm0, [rax + 32]
mov rdi, 0
lahf
mov rdi, rax

; Test 6: +Inf == +Inf
mov rax, 0xe0000000
movsd xmm0, [rax + 40]
ucomisd xmm0, [rax + 40]
mov r8, 0
lahf
mov r8, rax

hlt
