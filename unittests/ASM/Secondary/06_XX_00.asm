%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x0000000000000000",
    "RBX": "0x4142434400000000",
    "RCX": "0x4142434445460000",
    "RDX": "0x5152535455560000"
  }
}
%endif

; SLDT is an instruction caught by UMIP on recent CPUs.
; The Linux kernel emulates this by returning zero typically.
; Alternatively if an LDT has been loaded, it returns an emulated number.
mov rax, 0x4142434445464748
mov rbx, 0x4142434445464748
mov rcx, 0x4142434445464748
mov rdx, 0x4142434445464748

; Nasm for some reason doesn't give 64-bit SDLT
db 0x48, 0x0f, 0x00, 0xc0 ; sldt rax

sldt ebx
sldt cx
sldt [rel .data]
mov rdx, [rel .data]

hlt

align 4096
.data:
dq 0x5152535455565758
