%ifdef CONFIG
{
  "RegData": {
    "MM0": "0x4054c664c2f837b5",
    "MM1": "0x40516053e2d6238e",
    "MM2": "0x4044836d86ec17ec",
    "MM3": "0x402a1e1c58255b03",
    "MM4": "0x401568e0c9d9d346",
    "MM5": "0x4035fe425aee6320",
    "MM6": "0x402359003eea209b",
    "MM7": "0x40154b7d41743e96",
    "XMM0":  ["0x4054c664c2f837b5", "0x40516053e2d6238e"],
    "XMM1":  ["0x4044836d86ec17ec", "0x402a1e1c58255b03"],
    "XMM2":  ["0x401568e0c9d9d346", "0x4035fe425aee6320"],
    "XMM3":  ["0x402359003eea209b", "0x40154b7d41743e96"],
    "XMM4":  ["0x403d075a31a4bdba", "0x4050a018bd66277c"],
    "XMM5":  ["0x40334ec17ebaf102", "0x4056d7404ea4a8c1"],
    "XMM6":  ["0x404439b5c7cd898b", "0x40497b136a400fbb"],
    "XMM7":  ["0x4040528bc169c23b", "0x4037f9ca18bd6627"],
    "XMM8":  ["0x4056a929888f861a", "0x403839b866e43aa8"],
    "XMM9":  ["0x4058bc1f212d7732", "0x4056cde5c91d14e4"]
  }
}
%endif

; FEX had a bug where immediate encoded shifts by zero would generate bad code on AArch64.

lea rdx, [rel .data]
movq mm0, [rdx + 8 * 0]
movq mm1, [rdx + 8 * 1]
movq mm2, [rdx + 8 * 2]
movq mm3, [rdx + 8 * 3]
movq mm4, [rdx + 8 * 4]
movq mm5, [rdx + 8 * 5]
movq mm6, [rdx + 8 * 6]
movq mm7, [rdx + 8 * 7]

movapd xmm0, [rdx + 16 * 0]
movapd xmm1, [rdx + 16 * 1]
movapd xmm2, [rdx + 16 * 2]
movapd xmm3, [rdx + 16 * 3]
movapd xmm4, [rdx + 16 * 4]
movapd xmm5, [rdx + 16 * 5]
movapd xmm6, [rdx + 16 * 6]
movapd xmm7, [rdx + 16 * 7]
movapd xmm8, [rdx + 16 * 8]
movapd xmm9, [rdx + 16 * 9]

; Test MMX first
psllw mm0, 0
pslld mm1, 0
psllq mm2, 0
psraw mm3, 0
psrad mm4, 0
psrlw mm5, 0
psrld mm6, 0
psrlq mm7, 0

; Now test XMM
psllw xmm0, 0
pslld xmm1, 0
psllq xmm2, 0
pslldq xmm3, 0
psraw xmm4, 0
psrad xmm5, 0
psrlw xmm6, 0
psrld xmm7, 0
psrlq xmm8, 0
pslldq xmm9, 0

hlt
align 16
; 512bytes of random data
.data:
dq 83.0999,69.50512,41.02678,13.05881,5.35242,21.9932,9.67383,5.32372,29.02872,66.50151,19.30764,91.3633,40.45086,50.96153,32.64489,23.97574,90.64316,24.22547,98.9394,91.21715,90.80143,99.48407,64.97245,74.39838,35.22761,25.35321,5.8732,90.19956,33.03133,52.02952,58.38554,10.17531,47.84703,84.04831,90.02965,65.81329,96.27991,6.64479,25.58971,95.00694,88.1929,37.16964,49.52602,10.27223,77.70605,20.21439,9.8056,41.29389,15.4071,57.54286,9.61117,55.54302,52.90745,4.88086,72.52882,3.0201,56.55091,71.22749,61.84736,88.74295,47.72641,24.17404,33.70564,96.71303
