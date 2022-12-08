%ifdef CONFIG
{
  "HostFeatures": ["AVX"],
  "RegData": {
      "XMM0": ["0xA76C4F06A12BFCE0", "0x9B80767F1E6A060F", "0xA76C4F06A12BFCE0", "0x9B80767F1E6A060F"]
  }
}
%endif

lea rdx, [rel .data]

vbroadcastf128 ymm0, [rdx]

hlt

align 32
.data:
dq 0xA76C4F06A12BFCE0
dq 0x9B80767F1E6A060F
dq 0xFFFFFFFFFFFFFFFF
dq 0xEEEEEEEEEEEEEEEE
