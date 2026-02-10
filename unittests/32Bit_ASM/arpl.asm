%ifdef CONFIG
{
  "RegData": {
    "RAX": "0xFFFF_0003",
    "RBX": "0xFFFF_0003",
    "RCX": "0",
    "RDX": "0x000000000000119c",
    "RSI": "0x0000000000000297"
  },
  "Mode": "32BIT"
}
%endif

%macro setonz 1
  setz cl
  mov [rel .data + (%1 * 2)], cl
%endmacro

mov edx, 0
mov ecx, 0
mov esp, 0xe000_1000

; Setup some flags
mov edi, 0

; Rest of the code after this sub only touches eflags.z
sub edi, 1

%assign i 0
%assign offset 0
%rep 4
  %assign j 0
  %rep 4
    mov ebx, 0xFFFF_0000 + i
    mov eax, 0xFFFF_0000 + j
    ; ZF = dst.RPL < src.RPL
    ; if (ZF) dst.RPL = src.RPL
    arpl ax, bx
    setonz offset
    %assign j j+1
    %assign offset offset+1
  %endrep
  %assign i i+1
%endrep

; Load flag state
; Ensures that ONLY ZF changed.
pushfd
mov esi, [esp]

; Calculate data
%assign j 0
%rep 16
  mov cl, [rel .data + (j * 2)]
  or edx, ecx
  shl edx, 1
%assign j j+1
%endrep

hlt

.full_flags:
dd 0x4CD7

align 4096
.data:
dw 16 dup (0)
