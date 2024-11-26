%ifdef CONFIG
{
  "RegData": {
    "RAX": "1"
  },
  "Mode": "32BIT"
}
%endif

; FEX had a bug where inverting CF to match the ABI when flushing the register cache didn't mark CF as possibly being set.
; This caused accesses relying on that flag to be set correctly to return wrong values.

mov esp, 0xe0000020
mov al, 3
mov cl, 2
mov ecx, 1
mov eax, 1

and al, cl ; Zeros CF, non-inverted
push ecx ; Triggers a register cache flush
inc eax ; Tries to preserve CF, but would encounter the bug and set it instead
jnb succ
mov eax, 0
hlt
succ:
mov eax, 1
hlt
