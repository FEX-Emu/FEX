%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x11111111",
    "RBX": "0x22222222",
    "RCX": "0x33333333",
    "RDX": "0x44444444",
    "RSI": "0x55555555",
    "RDI": "0x66666666",
    "MM0": "0x1112131415161718",
    "MM1": "0x2122232425262728",
    "MM2": "0x3132333435363738",
    "MM3": "0x4142434445464748",
    "MM4": "0x5152535455565758",
    "MM5": "0x6162636465666768",
    "MM6": "0x7172737475767778",
    "MM7": "0x8182838485868788",
    "XMM0":  ["0x1112131415161718", "0x0"],
    "XMM1":  ["0x2122232425262728", "0x0"],
    "XMM2":  ["0x3132333435363738", "0x0"],
    "XMM3":  ["0x4142434445464748", "0x0"],
    "XMM4":  ["0x5152535455565758", "0x0"],
    "XMM5":  ["0x6162636465666768", "0x0"],
    "XMM6":  ["0x7172737475767778", "0x0"],
    "XMM7":  ["0x8182838485868788", "0x0"]
  },
  "Mode": "32BIT"

}
%endif

mov esp, 0xe0000000
mov ebp, 0xe0000500

; Set up MMX state
mov eax, 0x11121314
mov ecx, 0x15161718
mov dword [ebp + 4], eax
mov dword [ebp + 0], ecx
movq mm0, qword [ebp]

mov eax, 0x21222324
mov ecx, 0x25262728
mov dword [ebp + 4], eax
mov dword [ebp + 0], ecx
movq mm1, qword [ebp]

mov eax, 0x31323334
mov ecx, 0x35363738
mov dword [ebp + 4], eax
mov dword [ebp + 0], ecx
movq mm2, qword [ebp]

mov eax, 0x41424344
mov ecx, 0x45464748
mov dword [ebp + 4], eax
mov dword [ebp + 0], ecx
movq mm3, qword [ebp]

mov eax, 0x51525354
mov ecx, 0x55565758
mov dword [ebp + 4], eax
mov dword [ebp + 0], ecx
movq mm4, qword [ebp]

mov eax, 0x61626364
mov ecx, 0x65666768
mov dword [ebp + 4], eax
mov dword [ebp + 0], ecx
movq mm5, qword [ebp]

mov eax, 0x71727374
mov ecx, 0x75767778
mov dword [ebp + 4], eax
mov dword [ebp + 0], ecx
movq mm6, qword [ebp]

mov eax, 0x81828384
mov ecx, 0x85868788
mov dword [ebp + 4], eax
mov dword [ebp + 0], ecx
movq mm7, qword [ebp]

; Setup XMM state
mov eax, 0x11121314
mov ecx, 0x15161718
mov dword [ebp + 4], eax
mov dword [ebp + 0], ecx
movsd xmm0, [ebp]

mov eax, 0x21222324
mov ecx, 0x25262728
mov dword [ebp + 4], eax
mov dword [ebp + 0], ecx
movsd xmm1, [ebp]

mov eax, 0x31323334
mov ecx, 0x35363738
mov dword [ebp + 4], eax
mov dword [ebp + 0], ecx
movsd xmm2, [ebp]

mov eax, 0x41424344
mov ecx, 0x45464748
mov dword [ebp + 4], eax
mov dword [ebp + 0], ecx
movsd xmm3, [ebp]

mov eax, 0x51525354
mov ecx, 0x55565758
mov dword [ebp + 4], eax
mov dword [ebp + 0], ecx
movsd xmm4, [ebp]

mov eax, 0x61626364
mov ecx, 0x65666768
mov dword [ebp + 4], eax
mov dword [ebp + 0], ecx
movsd xmm5, [ebp]

mov eax, 0x71727374
mov ecx, 0x75767778
mov dword [ebp + 4], eax
mov dword [ebp + 0], ecx
movsd xmm6, [ebp]

mov eax, 0x81828384
mov ecx, 0x85868788
mov dword [ebp + 4], eax
mov dword [ebp + 0], ecx
movsd xmm7, [ebp]

; Corrupt state and see what it stores
mov eax, 0x41424344

; Overwrite header
mov dword [esp + 0], eax
; Overwrite the mm state
mov eax, -1
mov dword [esp + 32 + 4 * 0], eax
mov dword [esp + 32 + 4 * 1], eax
mov dword [esp + 32 + 4 * 2], eax
mov dword [esp + 32 + 4 * 3], eax
mov dword [esp + 32 + 4 * 4], eax
mov dword [esp + 32 + 4 * 5], eax
mov dword [esp + 32 + 4 * 6], eax
mov dword [esp + 32 + 4 * 7], eax
mov dword [esp + 32 + 4 * 8], eax
mov dword [esp + 32 + 4 * 9], eax
mov dword [esp + 32 + 4 * 10], eax
mov dword [esp + 32 + 4 * 11], eax
mov dword [esp + 32 + 4 * 12], eax
mov dword [esp + 32 + 4 * 13], eax
mov dword [esp + 32 + 4 * 14], eax
mov dword [esp + 32 + 4 * 15], eax

; Overwrite the xmm state
mov dword [esp + 160 + 4 * 0], eax
mov dword [esp + 160 + 4 * 1], eax
mov dword [esp + 160 + 4 * 2], eax
mov dword [esp + 160 + 4 * 3], eax
mov dword [esp + 160 + 4 * 4], eax
mov dword [esp + 160 + 4 * 5], eax
mov dword [esp + 160 + 4 * 6], eax
mov dword [esp + 160 + 4 * 7], eax
mov dword [esp + 160 + 4 * 8], eax
mov dword [esp + 160 + 4 * 9], eax
mov dword [esp + 160 + 4 * 10], eax
mov dword [esp + 160 + 4 * 11], eax
mov dword [esp + 160 + 4 * 12], eax
mov dword [esp + 160 + 4 * 13], eax
mov dword [esp + 160 + 4 * 14], eax
mov dword [esp + 160 + 4 * 15], eax
mov dword [esp + 160 + 4 * 16], eax
mov dword [esp + 160 + 4 * 17], eax
mov dword [esp + 160 + 4 * 18], eax
mov dword [esp + 160 + 4 * 19], eax
mov dword [esp + 160 + 4 * 20], eax
mov dword [esp + 160 + 4 * 21], eax
mov dword [esp + 160 + 4 * 22], eax
mov dword [esp + 160 + 4 * 23], eax
mov dword [esp + 160 + 4 * 24], eax
mov dword [esp + 160 + 4 * 25], eax
mov dword [esp + 160 + 4 * 26], eax
mov dword [esp + 160 + 4 * 27], eax
mov dword [esp + 160 + 4 * 28], eax
mov dword [esp + 160 + 4 * 29], eax
mov dword [esp + 160 + 4 * 30], eax
mov dword [esp + 160 + 4 * 31], eax
mov dword [esp + 160 + 4 * 32], eax
mov dword [esp + 160 + 4 * 33], eax
mov dword [esp + 160 + 4 * 34], eax
mov dword [esp + 160 + 4 * 35], eax
mov dword [esp + 160 + 4 * 36], eax
mov dword [esp + 160 + 4 * 37], eax
mov dword [esp + 160 + 4 * 38], eax
mov dword [esp + 160 + 4 * 39], eax
mov dword [esp + 160 + 4 * 40], eax
mov dword [esp + 160 + 4 * 41], eax
mov dword [esp + 160 + 4 * 42], eax
mov dword [esp + 160 + 4 * 43], eax
mov dword [esp + 160 + 4 * 44], eax
mov dword [esp + 160 + 4 * 45], eax
mov dword [esp + 160 + 4 * 46], eax
mov dword [esp + 160 + 4 * 47], eax
mov dword [esp + 160 + 4 * 48], eax
mov dword [esp + 160 + 4 * 49], eax
mov dword [esp + 160 + 4 * 50], eax
mov dword [esp + 160 + 4 * 51], eax
mov dword [esp + 160 + 4 * 52], eax
mov dword [esp + 160 + 4 * 53], eax
mov dword [esp + 160 + 4 * 54], eax
mov dword [esp + 160 + 4 * 55], eax
mov dword [esp + 160 + 4 * 56], eax
mov dword [esp + 160 + 4 * 57], eax
mov dword [esp + 160 + 4 * 58], eax
mov dword [esp + 160 + 4 * 59], eax
mov dword [esp + 160 + 4 * 60], eax
mov dword [esp + 160 + 4 * 61], eax
mov dword [esp + 160 + 4 * 62], eax
mov dword [esp + 160 + 4 * 63], eax

; Overwrite the three reserved 16byte elements
mov dword [esp + 416 + 4 * 0], eax
mov dword [esp + 416 + 4 * 1], eax
mov dword [esp + 416 + 4 * 2], eax
mov dword [esp + 416 + 4 * 3], eax
mov dword [esp + 416 + 4 * 4], eax
mov dword [esp + 416 + 4 * 5], eax
mov dword [esp + 416 + 4 * 6], eax
mov dword [esp + 416 + 4 * 7], eax
mov dword [esp + 416 + 4 * 8], eax
mov dword [esp + 416 + 4 * 9], eax
mov dword [esp + 416 + 4 * 10], eax
mov dword [esp + 416 + 4 * 11], eax

; Overwrite the three 16byte "available" slots
mov eax, 0x11111111
mov dword [esp + 464 + 4 * 0], eax
mov dword [esp + 464 + 4 * 1], eax
mov eax, 0x22222222
mov dword [esp + 464 + 4 * 2], eax
mov dword [esp + 464 + 4 * 3], eax
mov eax, 0x33333333
mov dword [esp + 464 + 4 * 4], eax
mov dword [esp + 464 + 4 * 5], eax
mov eax, 0x44444444
mov dword [esp + 464 + 4 * 6], eax
mov dword [esp + 464 + 4 * 7], eax
mov eax, 0x55555555
mov dword [esp + 464 + 4 * 8], eax
mov dword [esp + 464 + 4 * 9], eax
mov eax, 0x66666666
mov dword [esp + 464 + 4 * 10], eax
mov dword [esp + 464 + 4 * 11], eax
; Now save our state
fxsave [esp]

; Corrupt MMX And XMM state
mov eax, -1
mov dword [ebp + 0], eax
mov dword [ebp + 4], eax
mov dword [ebp + 8], eax
mov dword [ebp + 12], eax

movq mm0, qword [ebp]
movq mm1, qword [ebp]
movq mm2, qword [ebp]
movq mm3, qword [ebp]
movq mm4, qword [ebp]
movq mm5, qword [ebp]
movq mm6, qword [ebp]
movq mm7, qword [ebp]

; Setup XMM state
movaps xmm0, [ebp]
movaps xmm1, [ebp]
movaps xmm2, [ebp]
movaps xmm3, [ebp]
movaps xmm4, [ebp]
movaps xmm5, [ebp]
movaps xmm6, [ebp]
movaps xmm7, [ebp]
; Now reload the state we just saved
fxrstor [esp]

; Load the three 16bytes of "available" slots to make sure it wasn't overwritten
; Can't view full range here
; Reserved can be overwritten regardless
mov eax, dword [esp + 464 + 4 * 0]
mov ebx, dword [esp + 464 + 4 * 2]
mov ecx, dword [esp + 464 + 4 * 4]
mov edx, dword [esp + 464 + 4 * 6]
mov esi, dword [esp + 464 + 4 * 8]
mov edi, dword [esp + 464 + 4 * 10]

hlt
