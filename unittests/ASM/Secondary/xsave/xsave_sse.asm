%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x1111111111111111",
    "RBX": "0x2222222222222222",
    "RCX": "0x3333333333333333",
    "RDX": "0x4444444444444444",
    "RSI": "0x5555555555555555",
    "RDI": "0x6666666666666666",
    "MM0": "0",
    "MM1": "0",
    "MM2": "0",
    "MM3": "0",
    "MM4": "0",
    "MM5": "0",
    "MM6": "0",
    "MM7": "0",
    "XMM0":  ["0x1112131415161718", "0xABFDEC3402932039"],
    "XMM1":  ["0x2122232425262728", "0xDEFCA93847392992"],
    "XMM2":  ["0x3132333435363738", "0xEADC3284ADCE9339"],
    "XMM3":  ["0x4142434445464748", "0x3987432929293847"],
    "XMM4":  ["0x5152535455565758", "0x3764583402983799"],
    "XMM5":  ["0x6162636465666768", "0xACDEFACDEFACDEFA"],
    "XMM6":  ["0x7172737475767778", "0x3459238471238023"],
    "XMM7":  ["0x8182838485868788", "0x9347239480289299"],
    "XMM8":  ["0xCCC2C3C4C5C6C7C8", "0x3949232903428479"],
    "XMM9":  ["0xA1AAA3A4A5A6A7A8", "0x3784769228479192"],
    "XMM10": ["0xF1F2FFF4F5F6F7F8", "0x758734629799389A"],
    "XMM11": ["0xE1E2E3EEE5E6E7E8", "0x3756438328472389"],
    "XMM12": ["0xD1D2D3D4DDD6D7D8", "0x3674823989ADEF73"],
    "XMM13": ["0xC1C2C3C4C5CCC7C8", "0xABCDEF3894335820"],
    "XMM14": ["0xB1B2B3B4B5B6BBB8", "0xADEADE3894353499"],
    "XMM15": ["0xA1A2A3A4A5A6A7AA", "0xABFD392482039840"]
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

; Now save our state (SSE only)
mov eax, 0b010
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
