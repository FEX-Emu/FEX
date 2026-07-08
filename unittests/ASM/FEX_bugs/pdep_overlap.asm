%ifdef CONFIG
{
  "RegData": {
    "RBX": "0xAAAAAAAAAAAAAAAA",
    "RBX": "0x4444444444444444",
    "RCX": "0x4444444444444444",
    "RDX": "0x5555555555555555",
    "R8":  "0x0303030303030303",
    "RSI": "0x3333333333333333",
    "R8":  "0x0303030303030303"
  }
}
%endif


; dest and mask overlap
mov rax, 0xAAAAAAAAAAAAAAAA
mov rbx, 0x5555555555555555

pdep rbx, rax, rbx


; dest and input overlap
mov rcx, 0xAAAAAAAAAAAAAAAA
mov rdx, 0x5555555555555555

pdep rcx, rcx, rdx


; input and mask overlap
mov rdi, 0xDEADBEEFCAFEBABE
mov rsi, 0x3333333333333333

pdep rdi, rsi, rsi


; dest, input, mask overlap
mov r8, 0x3333333333333333

pdep r8, r8, r8


hlt