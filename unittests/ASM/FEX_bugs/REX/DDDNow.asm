%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x0506070801020304",
    "MM0": "0x0506070801020304"
  },
  "HostFeatures": ["3DNOW"]
}
%endif

femms
mov rax, 0x4142434445464748

mov r8, 0

lea rbx, [rel .data]
jmp .test
.test:

; pswapd mm0, [rbx]
; Real encoding: 0x0f, 0x0f, 0x03, 0xbb
; Add a NOP REX encoding between a volatile REX and the 3DNow! instruction.
; FEX accidentally being cumulative will cause rbx to convert to r8.
db 0x41, 0x40, 0x0f, 0x0f, 0x03, 0xbb

movd rax, mm0
hlt

align 16
.data:
dq 0x0102030405060708
