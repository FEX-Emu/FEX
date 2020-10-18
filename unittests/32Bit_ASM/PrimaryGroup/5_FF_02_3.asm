%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x41424344"
  },
  "MemoryRegions": {
    "0x80000000": "4096"
  },
  "Mode": "32BIT"
}
%endif

mov edi, 0xe0000000
lea esp, [edi + 8 * 4]

; Before we do anything, copy the code to an address that can be zexted
mov eax, dword [.inst_data]
mov [0x80000000], eax
mov eax, dword [.inst_data2]
mov [0x80000004], eax

mov eax, 0x41424344
mov [edi + 8 * 0], eax
mov eax, 0x51525354
mov [edi + 8 * 1], eax

mov eax, 0
db 0xFF
db 0x15
dd 0x80000004
hlt

.inst_data:
; mov eax, dword [edi]
; retn
db `\x8B\x07\xC3`
db 0
.inst_data2:
dd 0x80000000
