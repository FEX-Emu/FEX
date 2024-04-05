%ifdef CONFIG
{
  "RegData": {
    "RBX": "0x6",
    "RAX": "1"
  }
}
%endif

; FEX had a bug where variable shifts modified PF but RCLSE ignored this,
; causing RCLSE to invalidly propagate earlier PF results.

; First set PF to odd
mov rcx, 0
add rcx, 1

; Now do a variable shift that will set PF to even
mov rbx, 3
mov cl, 1
shl rbx, cl

; Save the PF. This should be 1 = even
setp al

; Trash NZCV. This means we'll optimize to calculate PF but not NZCV, which lets
; more constant prop happen needed to materialize the bug. This instruction is
; otherwise a no-op, but without it we pass by chance.
add rdx, rdx

hlt
