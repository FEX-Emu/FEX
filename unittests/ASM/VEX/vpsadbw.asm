%ifdef CONFIG
{
  "HostFeatures": ["AVX"],
  "RegData": {
    "XMM0":  ["0x100", "0x100", "0x000", "0x000"],
    "XMM8":  ["0x327", "0x2F4", "0x000", "0x000"],
    "XMM9":  ["0x2BA", "0x27F", "0x000", "0x000"],
    "XMM10": ["0x2B1", "0x284", "0x000", "0x000"],
    "XMM11": ["0x295", "0x280", "0x000", "0x000"],
    "XMM12": ["0x190", "0x279", "0x000", "0x000"],
    "XMM13": ["0x29B", "0x2A8", "0x000", "0x000"],
    "XMM14": ["0x25B", "0x1EA", "0x000", "0x000"]
  }
}
%endif

lea rdx, [rel .reg_data]

vmovaps xmm0, [rdx + 8 * 2]
psadbw xmm0, [rdx + 8 * 0]

lea rdx, [rel .data]

vmovaps xmm1, [rdx + 16 * 0]
vmovaps xmm2, [rdx + 16 * 1]
vmovaps xmm3, [rdx + 16 * 2]
vmovaps xmm4, [rdx + 16 * 3]
vmovaps xmm5, [rdx + 16 * 4]
vmovaps xmm6, [rdx + 16 * 5]
vmovaps xmm7, [rdx + 16 * 6]

vpsadbw xmm8,  xmm1, [rdx + 16 * 8]
vpsadbw xmm9,  xmm2, [rdx + 16 * 9]
vpsadbw xmm10, xmm3, [rdx + 16 * 10]
vpsadbw xmm11, xmm4, [rdx + 16 * 11]
vpsadbw xmm12, xmm5, [rdx + 16 * 12]
vpsadbw xmm13, xmm6, [rdx + 16 * 13]
vpsadbw xmm14, xmm7, [rdx + 16 * 14]

hlt

align 16

.reg_data:
dq 0x4142434445464748
dq 0x5152535455565758

dq 0x6162636465666768
dq 0x7172737475767778

.data:
dq 0xE0FC2BA1064F6CA7
dq 0x0F066A1E7F76809B

dq 0xE056EDAAF3C36868
dq 0xDEE6E694E2E9FCF0

dq 0x6E35A854D7AB8B6C
dq 0x775F92CA25A67E27

dq 0xC7CD73EC95D66F6A
dq 0xBBAEF2BB27B9A1DD

dq 0x734DD1C7D52C3188
dq 0xFEE7DBFD1E1E097F

dq 0x14FA4E95EFE69AF2
dq 0xA042629AA4A87382

dq 0x0E0F168238071232
dq 0x073592C1630778B3

dq 0xCB4619572B372A46
dq 0x1F040E793DCD8DA3

dq 0x2BF3862FABBA5730
dq 0x2ED62CF0464F3FEF

dq 0xEFD1BB85344B3CDE
dq 0x9E48A3B98D71E39D

dq 0x0972FBDE8A32509D
dq 0x6998F1F652EBF7EE

dq 0xD699C2FF301C02CE
dq 0x7005B2F1569C0EA6

dq 0x1862C4E286387630
dq 0x2FA1E4A70E5D53EB

dq 0x1445E0B7E1E80268
dq 0x1AFE8EC18FF2EB46

dq 0x7F5D6A2346972E03
dq 0x9812328F547659AC

dq 0xC8765FC8710CD3B6
dq 0xC519EAABA62C1D88
