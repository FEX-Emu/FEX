%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x0000000000011000",
    "RCX": "0x0000000051529654",
    "RDX": "0x0000000061626303"
  },
  "Mode": "32BIT"
}
%endif

; FEX-Emu had a bug with its `TelemetrySetValue` IR operation where it would corrupt host flags at an inopportune time.
; The IR operation does `cmp+cset`, but even with `ImplicitFlagClobber` set, this happened at a invalid time for flag handling.
; To test this:
;  - btr -> Sets CF
;  - adc with `ss:` -> Adds to register with carry, but `ss:` causes `TelemetrySetValue`.
;  - Host flags are corrupted after the `TelemetrySetValue`, before the `adc` was able to operate.

mov ecx, 0x51525354
mov edx, 0x61626303

lea eax, [.data]
lea esp, [.data_flags]
popf

and word [eax], dx
btr cx, dx
adc cx, ss:[eax]

hlt

align 4096

.data:
dd 0x41424344

.data_flags:
dd 0xfeff
