%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x1111111111111111",
    "RBX": "0x2222222222222222",
    "RCX": "0x3333333333333333",
    "RDX": "0x4444444444444444",
    "RSI": "0x5555555555555555",
    "RDI": "0x6666666666666666",
    "MM0": "0x1112131415161718",
    "MM1": "0x2122232425262728",
    "MM2": "0x3132333435363738",
    "MM3": "0x4142434445464748",
    "MM4": "0x5152535455565758",
    "MM5": "0x6162636465666768",
    "MM6": "0x7172737475767778",
    "MM7": "0x8182838485868788",
    "XMM0":  ["0", "0"],
    "XMM1":  ["0", "0"],
    "XMM2":  ["0", "0"],
    "XMM3":  ["0", "0"],
    "XMM4":  ["0", "0"],
    "XMM5":  ["0", "0"],
    "XMM6":  ["0", "0"],
    "XMM7":  ["0", "0"],
    "XMM8":  ["0", "0"],
    "XMM9":  ["0", "0"],
    "XMM10": ["0", "0"],
    "XMM11": ["0", "0"],
    "XMM12": ["0", "0"],
    "XMM13": ["0", "0"],
    "XMM14": ["0", "0"],
    "XMM15": ["0", "0"]
  },
  "HostFeatures": ["XSAVE"]
}
%endif

%include "xsave_macros.mac"

mov rsp, 0xE0000000

; Set up MMX and XMM state
set_up_mmx_state .xmm_data
set_up_xmm_state .xmm_data

overwrite_fxsave_slots

; Now save our state (X87 only)
mov eax, 0b001
xsave [rsp]

; Corrupt MMX And XMM state
corrupt_mmx_and_xmm_registers

; Now reload the state we just saved
xrstor [rsp]

; Load the three 16bytes of "available" slots to make sure it wasn't overwritten
; Reserved can be overwritten regardless
load_fxsave_slots

hlt

; Give ourselves a region of 1000 bytes set to 0xFF
align 64
.xsave_data:
  times 1000 db 0xFF

define_xmm_data_section
