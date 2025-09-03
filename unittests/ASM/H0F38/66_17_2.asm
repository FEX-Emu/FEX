%ifdef CONFIG
{
  "RegData": {
    "RAX": "0xFFFFFFFFFFFF43FF",
    "RBX": "0"
  },
  "HostFeatures": ["SSE4.1"]
}
%endif

; Set EFLAGS to known value with sahf
mov rax, -1
sahf

movups xmm0, [rel .data]
; Tests a bug that FEX had where ptest would not set OF, SF, AF, PF to zero
ptest xmm0, xmm0

; Now load back
; ZF = 1
; CF = 1
; OF, SF, AF, PF should be zero
lahf

; lahf doesn't get OF, get it with seto
mov rbx, 0
seto bl

hlt

.data:
dq 0
dq 0
