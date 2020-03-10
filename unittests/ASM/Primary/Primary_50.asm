%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x1",
    "RBX": "0x2",
    "RCX": "0x3",
    "RDX": "0x4",
    "RBP": "0x5",
    "RSI": "0x6",
    "RDI": "0x7",
    "R15": "0x8"
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

mov r15, 0xe0000080

mov rax, 1
mov rbx, 2
mov rcx, 3
mov rdx, 4
mov rbp, 5
mov rsi, 6
mov rdi, 7
mov r15, 8

push r15 ; Sub for rsp
push rdi
push rsi
push rbp
push rdx
push rcx
push rbx
push rax

mov rax, 0
mov rbx, 0
mov rcx, 0
mov rdx, 0
mov rbp, 0
mov rsi, 0
mov rdi, 0
mov r15, 0

pop rax
pop rbx
pop rcx
pop rdx
pop rbp
pop rsi
pop rdi
pop r15

hlt
