%ifdef CONFIG
{
  "HostFeatures": ["AVX"],
  "RegData": {
    "XMM0":  ["0x4047dcfb00bcbe62", "0x40382c8de2ac3223", "0x4040da5269595fee", "0x40582da24894c448"],
    "XMM1":  ["0x404c46843808850a", "0x4051ce8f32378ab1", "0x404eec764adff823", "0x40562f8c7e28240b"],
    "XMM2":  ["0x404a7427525460aa", "0x4013860029f16b12", "0x405221d82fd75e20", "0x4008292a30553261"],
    "XMM3":  ["0x402ed06f69446738", "0x404cc57c6fbd273d", "0x402338eb463497b7", "0x404bc581adea8976"],
    "XMM4":  ["0x40536d2fec56d5d0", "0x403436e2435696e6", "0x40239c779a6b50b1", "0x4044a59e30014f8b"],
    "XMM5":  ["0x40560c58793dd97f", "0x404295b6c3760bf6", "0x4048c3549f94855e", "0x40248b61bb05faec"],
    "XMM6":  ["0x405811ea0ba1f4b2", "0x401a9443d46b26c0", "0x403996f73c0c1fc9", "0x4057c071b4784231"],
    "XMM7":  ["0x4047ec6b7aa25d8d", "0x4055031782d38477", "0x405681e5c91d14e4", "0x4050740cf1800a7c"],
    "XMM10": ["0x40560c58793dd97f", "0x404295b6c3760bf6", "0x4048c3549f94855e", "0x40248b61bb05faec"],
    "XMM11": ["0x40536d2fec56d5d0", "0x403436e2435696e6", "0x40239c779a6b50b1", "0x4044a59e30014f8b"],
    "XMM12": ["0x402ed06f69446738", "0x404cc57c6fbd273d", "0x402338eb463497b7", "0x404bc581adea8976"],
    "XMM13": ["0x404a7427525460aa", "0x4013860029f16b12", "0x405221d82fd75e20", "0x4008292a30553261"],
    "XMM14": ["0x404c46843808850a", "0x4051ce8f32378ab1", "0x404eec764adff823", "0x40562f8c7e28240b"],
    "XMM15": ["0x4047dcfb00bcbe62", "0x40382c8de2ac3223", "0x4040da5269595fee", "0x40582da24894c448"]
  }
}
%endif

; FEX had a bug where immediate encoded shifts by zero would generate bad code on AArch64.

lea rdx, [rel .data]
vmovapd ymm0, [rdx + 32 * 0]
vmovapd ymm1, [rdx + 32 * 1]
vmovapd ymm2, [rdx + 32 * 2]
vmovapd ymm3, [rdx + 32 * 3]
vmovapd ymm4, [rdx + 32 * 4]
vmovapd ymm5, [rdx + 32 * 5]
vmovapd ymm6, [rdx + 32 * 6]
vmovapd ymm7, [rdx + 32 * 7]
vmovapd ymm8, [rdx + 32 * 8]
vmovapd ymm9, [rdx + 32 * 9]
vmovapd ymm10, [rdx + 32 * 10]
vmovapd ymm11, [rdx + 32 * 11]
vmovapd ymm12, [rdx + 32 * 12]
vmovapd ymm13, [rdx + 32 * 13]
vmovapd ymm14, [rdx + 32 * 14]
vmovapd ymm15, [rdx + 32 * 15]

vpsllw ymm0, ymm15, 0
vpslld ymm1, ymm14, 0
vpsllq ymm2, ymm13, 0
vpslldq ymm3, ymm12, 0
vpsraw ymm4, ymm11, 0
vpsrad ymm5, ymm10, 0
vpsrlw ymm6, ymm9, 0
vpsrld ymm7, ymm8, 0
vpsrlq ymm8, ymm7, 0
vpslldq ymm9, ymm6, 0

hlt
align 16
; 512bytes of random data
.data:
dq 83.0999,69.50512,41.02678,13.05881,5.35242,21.9932,9.67383,5.32372,29.02872,66.50151,19.30764,91.3633,40.45086,50.96153,32.64489,23.97574,90.64316,24.22547,98.9394,91.21715,90.80143,99.48407,64.97245,74.39838,35.22761,25.35321,5.8732,90.19956,33.03133,52.02952,58.38554,10.17531,47.84703,84.04831,90.02965,65.81329,96.27991,6.64479,25.58971,95.00694,88.1929,37.16964,49.52602,10.27223,77.70605,20.21439,9.8056,41.29389,15.4071,57.54286,9.61117,55.54302,52.90745,4.88086,72.52882,3.0201,56.55091,71.22749,61.84736,88.74295,47.72641,24.17404,33.70564,96.71303
