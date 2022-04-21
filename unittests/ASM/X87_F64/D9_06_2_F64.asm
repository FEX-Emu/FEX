%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x4000000000000000",
    "RBX": "0x3ff0000000000000"
  },
  "Env": { "FEX_X87REDUCEDPRECISION" : "1" }
}
%endif

mov rdx, 0xe0000000

mov eax, 0x3f800000 ; 1.0
mov [rdx + 8 * 0], eax
mov eax, 0x40000000 ; 2.0
mov [rdx + 8 * 1], eax
mov eax, 0x40800000 ; 4.0
mov [rdx + 8 * 2], eax

fld dword [rdx + 8 * 0]
o32 fstenv [rdx + 8 * 3]
fld dword [rdx + 8 * 2]
o32 fldenv [rdx + 8 * 3]

; This will overwrite the previous load
; This is since the control word is stored and reloaded
fld dword [rdx + 8 * 1]

; 14 bytes for 16bit
; 2 Bytes : FCW
; 2 Bytes : FSW
; 2 bytes : FTW
; 2 bytes : Instruction offset
; 2 bytes : Instruction CS selector
; 2 bytes : Data offset
; 2 bytes : Data selector

; 28 bytes for 32bit
; 4 bytes : FCW
; 4 bytes : FSW
; 4 bytes : FTW
; 4 bytes : Instruction pointer
; 2 bytes : instruction pointer selector
; 2 bytes : Opcode
; 4 bytes : data pointer offset
; 4 bytes : data pointer selector

fstp qword [rdx + 8]
mov rax, [rdx + 8]
fst qword [rdx + 8]
mov rbx, [rdx + 8]

hlt
