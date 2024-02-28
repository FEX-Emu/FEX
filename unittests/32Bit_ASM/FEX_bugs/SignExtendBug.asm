%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x41424344",
    "RBX": "0x41424344"
  },
  "MemoryRegions": {
    "0xf0000000": "4096"
  },
  "MemoryData": {
    "0xf0000000": "0x41424344"
  },
  "Mode": "32BIT"
}
%endif

; Ensures that zero extension of addresses are adhered to.
lea eax, [0xf000_0000]
mov eax, [ds:eax]

; Ensures that zext occurs correctly with two registers that have the sign bit set.
mov ebx, 0xffff_ffff
mov ecx, 0xf000_0001

; Break the block so it can't optimize through.
jmp .test
.test:
mov ebx, [ebx+ecx]

hlt
