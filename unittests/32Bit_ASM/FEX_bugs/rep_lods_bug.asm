%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x52",
    "RBX": "0x202",
    "RCX": "0"
  },
  "Mode": "32BIT"
}
%endif

; FEX had a bug that only manifests in 32-bit mode around pushing and popping flags around rep lobs{b,w,d,q}
; This manifested as a corrupt CF and ZF flag even though rep lodsb isn't supposed to affect flags.
; Test this by first storing zero to eflags, doing the operation and then loading it back.
mov esi, 0xe000_0000
mov esp, 0xe000_0800

mov eax, 0x41424344
mov [esi], eax

mov eax, 0x51525354
mov [esi + 4], eax

mov eax, 0
mov ecx, 7

; Push zero and then load back in to eflags.
push dword 0
popfd

; Do a rep lodsb, whichever size, doesn't matter.
rep lodsb

; Push flags and then load back in to ebx
pushfd
pop dword ebx

hlt
