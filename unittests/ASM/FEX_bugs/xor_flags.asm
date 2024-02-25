%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x0000000000004600"
  }
}
%endif

; FEX had a bug where an optimization for canonical zeroing of a register would fail to set flags correctly.
; This broke `Metal Gear Rising: Revengeance`. The title screen geometry was broken.

mov rax, 0
mov rbx, 0
sahf
xor rbx, rbx
lahf
hlt
