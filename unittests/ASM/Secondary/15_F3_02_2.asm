%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x0000434445464748",
    "RBX": "0x00000000FFFFFFFF"
  }
}
%endif

; Save FS
rdfsbase rax
mov [rel .data_backup], rax

mov rax, 0x0000434445464748
mov rbx, -1

; Ensure that wrfsbase of 32-bit will zero extend
wrfsbase rax
wrfsbase ebx
rdfsbase rbx ; 64bit

; Restore FS
mov rcx, [rel .data_backup]
wrfsbase rcx

hlt

align 4096
.data_backup:
dq 0
