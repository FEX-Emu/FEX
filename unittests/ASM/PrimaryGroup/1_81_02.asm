%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x4142A4A7A6A84748",
    "RBX": "0x51525354181B1E21",
    "RCX": "0x61626365282B2E31",
    "RDX": "0x6162636465666569"
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

mov rdx, 0xe0000000

mov rax, 0x4142434445464748
mov [rdx + 8 * 0], rax
mov rax, 0x5152535455565758
mov [rdx + 8 * 1], rax
mov rax, 0x6162636465666768
mov [rdx + 8 * 2], rax
mov rax, 0x6162636465666768
mov [rdx + 8 * 3], rax

clc
adc word [rdx + 8 * 0 + 2], 0x6162
clc
adc dword [rdx + 8 * 1 + 0], 0x61626364
clc
adc qword [rdx + 8 * 2 + 0], 0x61626364
clc
adc qword [rdx + 8 * 3 + 0], -256

stc
adc word [rdx + 8 * 0 + 4], 0x6162
stc
adc dword [rdx + 8 * 1 + 0], 0x61626364
stc
adc qword [rdx + 8 * 2 + 0], 0x61626364
stc
adc qword [rdx + 8 * 3 + 0], -256

mov rax, [rdx + 8 * 0]
mov rbx, [rdx + 8 * 1]
mov rcx, [rdx + 8 * 2]
mov rdx, [rdx + 8 * 3]

hlt
