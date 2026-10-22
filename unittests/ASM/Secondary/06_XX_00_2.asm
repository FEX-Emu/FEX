%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x0000000000000050",
    "RBX": "0x4142434400000050",
    "RCX": "0x4142434445460050",
    "RDX": "0x5152535455560050"
  },
  "HostFeatures": ["Linux"]
}
%endif

mov rsp, 0xe000_0080

; SLDT is an instruction caught by UMIP on recent CPUs.
; The Linux kernel emulates this by returning zero typically.
; Alternatively if an LDT has been loaded, it returns an emulated number.

mov rdi, 0x11 ; func LDT_WRITE
lea rsi, [rel .local_ldt]
mov rdx, 16 ; sizeof(struct user_desc)

lea rax, [rel .modify_ldt]
call rax

; Install an LDT entry and see what Linux returns.
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

; Pass arguments in SysV
.modify_ldt: ; (rdi:int:func, rsi:void*:ptr, rdx:ulong:bytecount)
  mov rax, 154 ; modify_ldt.
  syscall
  ret

align 4096
.data:
dq 0x5152535455565758

; Generate a user_desc manually.
.local_ldt:
dd 1  ; entry_number
dd 0  ; base_addr
dd -1 ; limit
dd (1 << 0) | (2 << 1) | (0 << 3) | (1 << 4) | (0 << 5) | (1 << 6) | (0 << 7)
