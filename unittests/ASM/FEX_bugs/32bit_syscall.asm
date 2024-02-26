%ifdef CONFIG
{
  "RegData": {
    "RAX": "0"
  }
}
%endif

; 32-bit:
; 265 = clock_gettime
; 64-bit
; 265 = linkat

; rax = syscall on both 32-bit and 64-bit
mov rax, 265

; rdi/rbx = first argument on 64-bit and 32-bit respectively
mov rdi, 0
mov rbx, 0

; rsi/rcx = second argument on 64-bit and 32-bit respectively
lea rsi, [rel .data]
lea rcx, [rel .data]

; Do a 32-bit syscall
; On a real linux kernel this will execute clock_gettime
; Under FEX without 32-bit syscall support this might try to execute linkat and return -ENOENT.
int 0x80
hlt

.data:
dq 0, 0, 0, 0
