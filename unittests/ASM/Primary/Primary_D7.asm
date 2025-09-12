%ifdef CONFIG
{
  "RegData": {
    "R15": "0xFFFFFFFFFFFFFF47",
    "R14": "0xFFFFFFFFFFFFFF57",
    "R13": "0xFFFFFFFFFFFFFF67"
  }
}
%endif

; Save FS/GS
rdfsbase rax
mov [rel .data_backup], rax
rdgsbase rax
mov [rel .data_backup + 8], rax

mov rbx, 0xe0000000
lea r9, [rbx + 8 * 1]
wrfsbase r9
lea r9, [rbx + 8 * 2]
wrgsbase r9

mov rcx, 0x4142434445464748
mov [rbx + 8 * 0], rcx
mov rcx, 0x5152535455565758
mov [rbx + 8 * 1], rcx
mov rcx, 0x6162636465666768
mov [rbx + 8 * 2], rcx

; Base
mov rax, 0xFFFFFFFFFFFFFF01
xlatb
mov r15, rax

; FS
mov rax, 0xFFFFFFFFFFFFFF01
mov rbx, 0
fs xlat
mov r14, rax

; GS
mov rax, 0xFFFFFFFFFFFFFF01
mov rbx, 0
gs xlat
mov r13, rax

; Restore FS/GS
mov rax, [rel .data_backup]
wrfsbase rax
mov rax, [rel .data_backup + 8]
wrgsbase rax

hlt

.data_backup:
dq 0
dq 0
