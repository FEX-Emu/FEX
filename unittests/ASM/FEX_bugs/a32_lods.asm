%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x4142434445464748",
    "RSI": "0x0000000000080008"
  },
  "MemoryRegions": {
    "0x000080000": "4096",
    "0x100000000": "4096"
  }
}
%endif

; FEX-Emu had a bug where it wouldn't do address-size override on lods.

; Address that overlaps the two memory regions depending on if you mask the upper bits or not.
; LODS operation needs to exist in rsi.
mov rsi, 0x1_0008_0000
mov rsp, 0x0_0008_0000
mov rdx, 0x1_0000_0000
mov rbx, 0x4142434445464748
mov rcx, 0x5152535455565758
mov rax, 0

mov [rsp], rbx
mov [rdx], rcx

; Load with address size override.
; Should truncate the upper bits of the address.
a32 lodsq

hlt
