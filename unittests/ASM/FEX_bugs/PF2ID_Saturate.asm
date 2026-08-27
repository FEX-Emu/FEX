%ifdef CONFIG
{
  "RegData": {
    "MM0": "0x800000007FFFFFFF"
  },
  "HostFeatures": ["3DNOW"]
}
%endif

; +3000000000.0f > INT32_MAX -> saturate to 0x7FFFFFFF
; -3000000000.0f < INT32_MIN -> saturate to 0x80000000
pf2id mm0, [rel data1]

hlt

align 8
data1:
dd 0x4F32D05E ; +3000000000.0f
dd 0xCF32D05E ; -3000000000.0f
