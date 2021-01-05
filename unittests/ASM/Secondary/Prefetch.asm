%ifdef CONFIG
{
}
%endif

; Arg 1 = type
; Arg 2 = Reg
%macro prefetch 2
  prefetch%1 [%2]
%endmacro

; Arg1 = prefix
; Arg2 = Reg
%macro prefetch_pre 2
  db %1
  prefetch nta, %2
  db %1
  prefetch t0, %2
  db %1
  prefetch t1, %2
  db %1
  prefetch t2, %2
%endmacro

; Arg 1 = modrm encoding
; Always uses rax
%macro prefetch_res 1
  db 0x0F
  db 0x18
  db %1
%endmacro

; Arg 1 = prefix
; Arg 2 = modrm encoding
%macro prefetch_res_pre 2
  db %1
  prefetch_res %2
%endmacro

; Arg 1 = modrm encoding
; Always uses rax
%macro prefetch_resw 1
  db 0x0F
  db 0x0D
  db %1
%endmacro

; Arg 1 = prefix
; Arg 2 = modrm encoding
%macro prefetch_resw_pre 2
  db %1
  prefetch_resw %2
%endmacro

mov rax, 0xe0000000

prefetch nta, rax
prefetch t0, rax
prefetch t1, rax
prefetch t2, rax

prefetch_pre 0x66, rax
prefetch_pre 0x66, rax
prefetch_pre 0x66, rax
prefetch_pre 0x66, rax

prefetch_pre 0xF2, rax
prefetch_pre 0xF2, rax
prefetch_pre 0xF2, rax
prefetch_pre 0xF2, rax

prefetch_pre 0xF3, rax
prefetch_pre 0xF3, rax
prefetch_pre 0xF3, rax
prefetch_pre 0xF3, rax

prefetch_res (0 << 3)
prefetch_res (1 << 3)
prefetch_res (2 << 3)
prefetch_res (3 << 3)
prefetch_res (4 << 3)
prefetch_res (5 << 3)
prefetch_res (6 << 3)
prefetch_res (7 << 3)

prefetch_res_pre 0x66, (0 << 3)
prefetch_res_pre 0x66, (1 << 3)
prefetch_res_pre 0x66, (2 << 3)
prefetch_res_pre 0x66, (3 << 3)
prefetch_res_pre 0x66, (4 << 3)
prefetch_res_pre 0x66, (5 << 3)
prefetch_res_pre 0x66, (6 << 3)
prefetch_res_pre 0x66, (7 << 3)

prefetch_res_pre 0xF2, (0 << 3)
prefetch_res_pre 0xF2, (1 << 3)
prefetch_res_pre 0xF2, (2 << 3)
prefetch_res_pre 0xF2, (3 << 3)
prefetch_res_pre 0xF2, (4 << 3)
prefetch_res_pre 0xF2, (5 << 3)
prefetch_res_pre 0xF2, (6 << 3)
prefetch_res_pre 0xF2, (7 << 3)

prefetch_res_pre 0xF3, (0 << 3)
prefetch_res_pre 0xF3, (1 << 3)
prefetch_res_pre 0xF3, (2 << 3)
prefetch_res_pre 0xF3, (3 << 3)
prefetch_res_pre 0xF3, (4 << 3)
prefetch_res_pre 0xF3, (5 << 3)
prefetch_res_pre 0xF3, (6 << 3)
prefetch_res_pre 0xF3, (7 << 3)


prefetch_resw (0 << 3)
prefetch_resw (1 << 3)
prefetch_resw (2 << 3)
prefetch_resw (3 << 3)
prefetch_resw (4 << 3)
prefetch_resw (5 << 3)
prefetch_resw (6 << 3)
prefetch_resw (7 << 3)

prefetch_resw_pre 0x66, (0 << 3)
prefetch_resw_pre 0x66, (1 << 3)
prefetch_resw_pre 0x66, (2 << 3)
prefetch_resw_pre 0x66, (3 << 3)
prefetch_resw_pre 0x66, (4 << 3)
prefetch_resw_pre 0x66, (5 << 3)
prefetch_resw_pre 0x66, (6 << 3)
prefetch_resw_pre 0x66, (7 << 3)

prefetch_resw_pre 0xF2, (0 << 3)
prefetch_resw_pre 0xF2, (1 << 3)
prefetch_resw_pre 0xF2, (2 << 3)
prefetch_resw_pre 0xF2, (3 << 3)
prefetch_resw_pre 0xF2, (4 << 3)
prefetch_resw_pre 0xF2, (5 << 3)
prefetch_resw_pre 0xF2, (6 << 3)
prefetch_resw_pre 0xF2, (7 << 3)

prefetch_resw_pre 0xF3, (0 << 3)
prefetch_resw_pre 0xF3, (1 << 3)
prefetch_resw_pre 0xF3, (2 << 3)
prefetch_resw_pre 0xF3, (3 << 3)
prefetch_resw_pre 0xF3, (4 << 3)
prefetch_resw_pre 0xF3, (5 << 3)
prefetch_resw_pre 0xF3, (6 << 3)
prefetch_resw_pre 0xF3, (7 << 3)


hlt
