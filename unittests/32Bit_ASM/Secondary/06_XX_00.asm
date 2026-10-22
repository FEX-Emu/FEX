%ifdef CONFIG
{
  "RegData": {
    "RBX": "0x0000000000000050",
    "RCX": "0x0000000041420050",
    "RDX": "0x0000000051520050"
  },
  "Mode": "32BIT"
}
%endif

; SLDT is an instruction caught by UMIP on recent CPUs.
; On 32-bit an LDT is already loaded in the test harness runner.
mov ebx, 0x41424344
mov ecx, 0x41424344
mov edx, 0x41424344

sldt ebx
sldt cx
sldt [rel .data]
mov edx, [rel .data]

hlt

align 4096
.data:
dq 0x51525354
