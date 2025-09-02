%ifdef CONFIG
{
  "RegData": {
    "RAX": "0xffffffff81828384",
    "RBX": "0xffffffff81828384",
    "RCX": "0x0000000081828384",
    "RDX": "0x4142434445468384"
  }
}
%endif

mov rax, 0x4142434445464748
mov rbx, 0x4142434445464748
mov rcx, 0x4142434445464748
mov rdx, 0x4142434445464748
mov rsp, 0x6666666681828384

; Default: 0x48, 0x63, 0xc4
movsxd rax, esp
; Default with o16 prefix: 0x66, 0x48, 0x63, 0xc4
o16 movsxd rbx, esp
; No-rex widening prefix
db 0x63, 0xcc ; movsxd ecx, esp
; o16 prefix with no-rex widening
db 0x66, 0x63, 0xd4 ; movsxd dx, sp

hlt
