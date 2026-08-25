%ifdef CONFIG
{
  "RegData": {
      "RAX": "1",
      "R15": "1",
      "R14": "1",
      "R13": "1",
      "R12": "1"
  }
}
%endif

; FEX-Emu had a bug where operand-sized shifts with SHLD/SHRD wasn't matching behaviour.
; We were expecting SAL/SAR/SHL/SHR undefined behaviour semantics but a `ge` comparison changes to `gt`.

; Test with shld and immediate
mov eax, 1
mov edx, 0xbee8
mov rcx, 0
clc
shld ax, dx, 16
setc cl
mov r15, rcx

; Test with shld and CL
mov eax, 1
mov edx, 0xbee8
mov rcx, 16
clc
shld ax, dx, cl
setc cl
mov r14, rcx

; Test with shrd and immediate
mov eax, 0x8eeb
mov edx, 1
mov rcx, 0
clc
shrd ax, dx, 16
setc cl
mov r13, rcx

; Test with shrd and CL
mov eax, 0x8eeb
mov edx, 1
mov rcx, 16
clc
shrd ax, dx, cl
setc cl
mov r12, rcx

hlt
