%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x4142434445464748",
    "RBX": "0x5758000000006768",
    "RCX": "0xE0000016"
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

mov r15, 0xe0000000
lea rsp, [r15 + 8 * 4]

mov rax, 0x4142434445464748
mov [r15 + 8 * 0], rax
mov rax, 0x5152535455565758
mov [r15 + 8 * 1], rax
mov rax, 0x6768
mov [r15 + 8 * 2], rax
mov [r15 + 8 * 3], rax

; Encoding doesn't exist in x86-64
; push dword [r15 + 8 * 1 + 0]
push qword [r15 + 8 * 0 + 0]
push  word [r15 + 8 * 1 + 0]

mov rax, [r15 + 8 * 3]
mov rbx, [r15 + 8 * 2]
mov rcx, rsp

hlt
