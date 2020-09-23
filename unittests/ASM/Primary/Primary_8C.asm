%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x4142",
    "RBX": "0x4143",
    "RCX": "0x4144"
  }
}
%endif
; This relies on some behaviour that isn't guaranteed in 64bit mode

; Technically this can result in an invalid selector which can cause faults
; We currently don't do any selector validation to enforce this
mov rax, 0x4142

mov es, ax

inc rax
mov ss, ax

inc rax
mov ds, ax

; Can't test FS/GS here
; Behaviour is ill-defined and needs to be worked through

mov rax, 0
mov rbx, 0
mov rcx, 0

mov ax, es
mov bx, ss
mov cx, ds

hlt
