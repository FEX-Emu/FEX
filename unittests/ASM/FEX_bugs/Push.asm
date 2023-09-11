%ifdef CONFIG
{
  "RegData": {
    "RAX": "0xe0000010",
    "RSP": "0xe0000008"
  }
}
%endif

; FEX had a bug where a `push rsp` would generate an Arm64 instruction with undefined behaviour.
; `push rsp` -> `str x8, [x8, #-8]!`
; This instruction has constrained undefined behaviour.
; On Cortex it stores the original value.
; On Apple Silicon it raises a SIGILL.
; It can also store undefined data or have undefined behaviour.
; Test to ensure we don't generate undefined behaviour.
mov rsp, 0xe0000010
push rsp

mov rax, [rsp]

hlt
