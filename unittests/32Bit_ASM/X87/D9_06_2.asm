%ifdef CONFIG
{
  "RegData": {
    "MM6":  ["0x8000000000000000", "0x4000"],
    "MM7":  ["0x8000000000000000", "0x3fff"]
  },
  "Mode": "32BIT"
}
%endif

lea edx, [rel .data]

fld dword [edx + 8 * 0]
o32 fstenv [edx + 8 * 3]
fld dword [edx + 8 * 2]
o32 fldenv [edx + 8 * 3]

; This will overwrite the previous load
; This is since the control word is stored and reloaded
fld dword [edx + 8 * 1]

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

hlt

align 4096
.data:
dq 0x3f800000
dq 0x40000000
dq 0x40800000
dq 0
dq 0
