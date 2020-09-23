%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x0600",
    "RCX": "0x5",
    "RDX": "0x0",
    "RDI": "0xE0000005",
    "RSI": "0xE0000015"
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

%macro copy 3
  ; Dest, Src, Size
  mov rdi, %1
  mov rsi, %2
  mov rcx, %3

  cld
  rep movsb
%endmacro

mov rdx, 0xe0000000

lea r15, [rdx + 8 * 0]
lea r14, [rel .StringOne]
copy r15, r14, 11

lea r15, [rdx + 8 * 2]
lea r14, [rel .StringTwo]
copy r15, r14, 14

lea rdi, [rdx + 8 * 0]
lea rsi, [rdx + 8 * 2]

cld
mov rcx, 10 ; Lower String length
repe cmpsb ; rdi cmp rsi
mov rax, 0
lahf

mov rdx, 0
sete dl

hlt

.StringOne: db "TestString\0"
.StringTwo: db "Test\0"
