%ifdef CONFIG
{
  "RegData": {
    "RAX": "0",
    "RBX": "1",
    "R8":  "1",
    "R9":  "0",
    "R10": "0",
    "R11": "1",
    "R12": "1",
    "R13": "1",
    "R14": "0x00000000ffffffff",
    "R15": "0"
  }
}
%endif

; FEX had a bug in pr #3153 which encountered a destination register overwrite in sbbNZCV.
; This was due to the destination register for that IR operation aliases the first source register.
; Once it tried modifying NZCV flags directly in the destination, it managed to clobber the source register.
; Code is based around part of a GCC adx-addcarryx32-2 assembly output snippet.
; Needs memory accesses to ensure const-prop and RA aligns correctly.
mov rdx, 0

; These need to be loaded through memory so const-prop doesn't save it
; Load the values
mov     edx, dword [rel .current_x]
mov     eax, dword [rel .current_y]
movzx   ecx, byte [rel .current_stored_cf]

; Do the operation
add     cl, 0xff ; Clear carry based on stored_cf. Can't use clc here. (0)
mov r8d, edx ; Store incoming current_x
mov r9d, eax ; Store incoming current_y
setb r10b ; Store incoming CF

sbb     eax, edx
; Get Carry result
setb    dl

setb r11b ; Store outgoing CF

; Store sbb result and carry
mov     dword [rel .current_x], eax ; (0xFFFF_FFFF)
mov     byte [rel .current_stored_cf], dl ; (0x1)

; Second operation
; Load current_y and CF(will be 1)
mov     eax, 0
movzx   edx, byte [rel .current_stored_cf]

movzx   r12, byte [rel .current_stored_cf] ; Store incoming CF

add     dl, 0xff ; Set carry based on stored_cf. Can't use stc here. (1)
; Do the operation

mov r15d, eax ; Store EAX prior to SBB
mov r14d, dword [rel .current_x] ; Store curent_x
setb r13b ; Store incoming CF

sbb     eax, dword [rel .current_x]
setb    dl  ; Set if CF=1

; sbb results in eax now
; Move carry result to ebx
; r15 = EAX prior to second sbb
; r14 = CurrentX prior to second sbb (-1)
; r13 = Incoming CF prior to second sbb (calculated from setb)
; r12 = Incoming CF prior to second sbb (calculated from memory)
; r11 = Outgoing CF from first sbb (calculated from setb)
; r10 = Incoming CF for first sbb (calculated from setb)
; r9 = store incoming current_y to first sbb
; r8 = store incoming current_x to first sbb

movzx ebx, dl

hlt

align 4096
.current_x:
dq 1

.current_y:
dq 0

.current_stored_cf:
dq 0
