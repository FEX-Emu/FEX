%ifdef CONFIG
{
  "RegData": {
    "R15": "0x3"
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

%macro cfmerge 0

; Get CF
sbb r14, r14
and r14, 1

; Merge in to results
shl r15, 1
or r15, r14

%endmacro

mov rdx, 0xe0000000

mov rax, 0x0000000100000000
mov [rdx + 8 * 0], rax

xor r15, r15 ; Will contain our results

bts qword [rdx], 32
cfmerge

bt qword [rdx], 32
cfmerge

hlt
