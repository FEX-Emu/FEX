%ifdef CONFIG
{
  "RegData": {
    "XMM0": ["0x0000458400004584", "0x4584850445848504"],
    "XMM1": ["0x0000458400004584", "0x35d4d55435d4d554"],
    "XMM2": ["0x0000868800008688", "0x8284868882848688"],
    "XMM3": ["0x0000868800008688", "0xaaacaeb0aaacaeb0"],
    "XMM4": ["0", "0"],
    "XMM5": ["0", "0"],
    "XMM6": ["0", "0"],
    "XMM7": ["0", "0"]
  }
}
%endif

; SHLD/SHRD is kind of quirky, where the CF contains the last bit shifted out of the /first/ operand.
; What does that mean when the shift is larger than the operand? Technically the result register is undefined.
; AMD documentation doesn't claim CF is undefined.
; Intel pseudocode claims flags AND result is undefined.

; SHLD first
%assign i 0
%rep 64
  mov ebx, 0x41424344
  mov ecx, 0x55565758
  mov eax, 0
  clc
  shld bx, cx, i
  setc al
  shl al, (i % 8)
  or [rel .data_result + (i / 8)], al
%assign i i+1
%endrep

%assign i 0
%rep 64
  mov ebx, 0x41424344
  mov ecx, 0x55565758
  mov eax, 0
  clc
  shld ebx, ecx, i
  setc al
  shl al, (i % 8)
  or [rel .data_result + (i / 8) + 8], al
%assign i i+1
%endrep

%assign i 0
%rep 64
  mov eax, 0x41424344
  mov ebx, 0x55565758
  mov cl, i
  mov edx, 0
  clc
  shld ax, bx, cl
  setc dl
  shl dl, (i % 8)
  or [rel .data_result + (i / 8) + 16], dl
%assign i i+1
%endrep

%assign i 0
%rep 64
  mov eax, 0x41424344
  mov ebx, 0x55565758
  mov cl, i
  mov edx, 0
  clc
  shld ebx, ecx, cl
  setc dl
  shl dl, (i % 8)
  or [rel .data_result + (i / 8) + 24], dl
%assign i i+1
%endrep

; SHRD second
%assign i 0
%rep 64
  mov ebx, 0x41424344
  mov ecx, 0x55565758
  mov eax, 0
  clc
  shrd bx, cx, i
  setc al
  shl al, (i % 8)
  or [rel .data_result + (i / 8) + 32], al
%assign i i+1
%endrep

%assign i 0
%rep 64
  mov ebx, 0x41424344
  mov ecx, 0x55565758
  mov eax, 0
  clc
  shrd ebx, ecx, i
  setc al
  shl al, (i % 8)
  or [rel .data_result + (i / 8) + 40], al
%assign i i+1
%endrep

%assign i 0
%rep 64
  mov eax, 0x41424344
  mov ebx, 0x55565758
  mov cl, i
  mov edx, 0
  clc
  shrd ax, bx, cl
  setc dl
  shl dl, (i % 8)
  or [rel .data_result + (i / 8) + 48], dl
%assign i i+1
%endrep

%assign i 0
%rep 64
  mov eax, 0x41424344
  mov ebx, 0x55565758
  mov cl, i
  mov edx, 0
  clc
  shrd ebx, ecx, cl
  setc dl
  shl dl, (i % 8)
  or [rel .data_result + (i / 8) + 56], dl
%assign i i+1
%endrep

movaps xmm0, [rel .data_result]
movaps xmm1, [rel .data_result + 16]
movaps xmm2, [rel .data_result + 32]
movaps xmm3, [rel .data_result + 48]

movaps xmm4, [rel .data_result2]
movaps xmm5, [rel .data_result2 + 16]
movaps xmm6, [rel .data_result2 + 32]
movaps xmm7, [rel .data_result2 + 48]

hlt

align 4096
.data_result:
times 8 dq 0

.data_result2:
times 8 dq 0
