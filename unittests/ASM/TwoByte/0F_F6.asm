%ifdef CONFIG
{
  "RegData": {
    "MM0": "0x0000000000000080",
    "MM1": "0x0000000000000083",
    "MM2": "0x0000000000000134",
    "MM3": "0x0000000000000156",
    "MM4": "0x0000000000000140",
    "MM5": "0x000000000000013F",
    "MM6": "0x000000000000008F",
    "MM7": "0x00000000000000D1"
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
mov rax, 0x912277A763B4EB8C
mov [rdx + 8 * 2], rax
mov rax, 0x589490D442F54AFD
mov [rdx + 8 * 3], rax
mov rax, 0xB5E43417A3F6706C
mov [rdx + 8 * 4], rax
mov rax, 0xB4F4B827515F5BFA
mov [rdx + 8 * 5], rax
mov rax, 0x52D0EF1BCB906B6A
mov [rdx + 8 * 6], rax
mov rax, 0x1D0FDF5D05D39C64
mov [rdx + 8 * 7], rax
mov rax, 0xAEFEDEA21EF08810
mov [rdx + 8 * 8], rax
mov rax, 0xF7D80319B125BDE5

movq mm0, [rdx + 8 * 1]
movq mm1, [rdx + 8 * 2]
movq mm2, [rdx + 8 * 3]
movq mm3, [rdx + 8 * 4]
movq mm4, [rdx + 8 * 5]
movq mm5, [rdx + 8 * 6]
movq mm6, [rdx + 8 * 7]
movq mm7, [rdx + 8 * 8]

psadbw mm0, [rdx + 8 * 0]

lea rdx, [rel .data]
movq mm1, [rdx + 8 * 0]
movq mm2, [rdx + 8 * 1]
movq mm3, [rdx + 8 * 2]
movq mm4, [rdx + 8 * 3]
movq mm5, [rdx + 8 * 4]
movq mm6, [rdx + 8 * 5]
movq mm7, [rdx + 8 * 6]

psadbw mm1, [rdx + 8 * 7]
psadbw mm2, [rdx + 8 * 8]
psadbw mm3, [rdx + 8 * 9]
psadbw mm4, [rdx + 8 * 10]
psadbw mm5, [rdx + 8 * 11]
psadbw mm6, [rdx + 8 * 12]
psadbw mm7, [rdx + 8 * 13]
hlt

.data:
; 128bytes of random numbers
db 'ba\xa7\x5e\xc8\x0f\x90\x25\xf1\xf8\x49\xbd\xab\x4d\x2b\xa1\xc4'
db 'e4\x69\xe3\x2a\x80\x8d\xd6\x0b\xb2\x6d\xea\xae\x2e\x23\xc2\x2c'
db 'f9\xc6\xee\x06\x53\x96\x00\xae\x8d\x06\xdc\xe1\x11\x06\x0c\x40'
db 'a5\x61\x83\x7c\x13\x25\x43\xea\xa7\x08\x52\xc4\x0f\x91\x2c\x2c'
db '5a\xe7\xcf\xf6\xe3\x6b\x9e\x9e\xd8\x85\xf7\xfd\x4a\x17\xb4\xc9'
db '16\x07\x13\x8c\x83\x89\xc3\x5e\x46\x63\x1a\x31\xb9\x2c\x72\x18'
db '23\xa2\xf0\x4d\x22\x2a\xe4\x86\x84\x1a\xae\xfc\x65\x49\x17\x8e'
db 'c8\xb0\xe3\x6c\xb3\xce\xa1\x2f\xce\x5f\xae\x06\xac\x28\x7d\xb5'
