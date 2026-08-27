%ifdef CONFIG
{
  "RegData": {
    "MM0": "0xFFFF800000007FFF"
  },
  "HostFeatures": ["3DNOW"]
}
%endif

; +100000.0f > INT16_MAX -> saturate to 0x00007FFF
; -100000.0f < INT16_MIN -> saturate to 0xFFFF8000
pf2iw mm0, [rel data1]

hlt

align 8
data1:
dd 0x47C35000 ; +100000.0f
dd 0xC7C35000 ; -100000.0f
