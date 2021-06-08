%ifdef CONFIG
{
  "RegData": {
    "XMM0": ["0x0000000042a63326", "0x0000000000000000"],
    "XMM1": ["0x0000000040ab4706", "0x0000000000000000"],
    "XMM2": ["0x0000000041e83ad2", "0x0000000000000000"],
    "XMM3": ["0x000000004221cdae", "0x0000000000000000"],
    "XMM4": ["0x0000000042b5494c", "0x0000000000000000"],
    "XMM5": ["0x0000000042b59a55", "0x0000000000000000"],
    "XMM6": ["0x00000000420ce913", "0x0000000000000000"],
    "XMM7": ["0x0000000042042015", "0x0000000000000000"],
    "XMM8": ["0x00000000423f635c", "0x0000000000000000"],
    "XMM9": ["0x0000000042c08f50", "0x0000000000000000"],
    "XMM10": ["0x0000000042b062c4", "0x0000000000000000"],
    "XMM11": ["0x00000000429b697f", "0x0000000000000000"],
    "XMM12": ["0x000000004176837b", "0x0000000000000000"],
    "XMM13": ["0x000000004253a13b", "0x0000000000000000"],
    "XMM14": ["0x0000000042623422", "0x0000000000000000"],
    "XMM15": ["0x00000000423ee7d8", "0x0000000000000000"]
  }
}
%endif

lea rdx, [rel .data]
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
movapd xmm10, [rdx + 16 * 10]
movapd xmm11, [rdx + 16 * 11]
movapd xmm12, [rdx + 16 * 12]
movapd xmm13, [rdx + 16 * 13]
movapd xmm14, [rdx + 16 * 14]
movapd xmm15, [rdx + 16 * 15]

mov rdx, 0xe0000000
mov rax, 0
mov [rdx + 8 * 0], rax
mov [rdx + 8 * 1], rax
mov [rdx + 8 * 2], rax
mov [rdx + 8 * 3], rax
mov [rdx + 8 * 4], rax
mov [rdx + 8 * 5], rax
mov [rdx + 8 * 6], rax
mov [rdx + 8 * 7], rax
mov [rdx + 8 * 8], rax
mov [rdx + 8 * 9], rax
mov [rdx + 8 * 10], rax
mov [rdx + 8 * 11], rax
mov [rdx + 8 * 12], rax
mov [rdx + 8 * 13], rax
mov [rdx + 8 * 14], rax
mov [rdx + 8 * 15], rax
mov [rdx + 8 * 16], rax
mov [rdx + 8 * 17], rax
mov [rdx + 8 * 18], rax
mov [rdx + 8 * 19], rax
mov [rdx + 8 * 20], rax
mov [rdx + 8 * 21], rax
mov [rdx + 8 * 22], rax
mov [rdx + 8 * 23], rax
mov [rdx + 8 * 24], rax
mov [rdx + 8 * 25], rax
mov [rdx + 8 * 26], rax
mov [rdx + 8 * 27], rax
mov [rdx + 8 * 28], rax
mov [rdx + 8 * 29], rax
mov [rdx + 8 * 30], rax

movss [rdx + 16 * 0], xmm0
movss [rdx + 16 * 1], xmm1
movss [rdx + 16 * 2], xmm2
movss [rdx + 16 * 3], xmm3
movss [rdx + 16 * 4], xmm4
movss [rdx + 16 * 5], xmm5
movss [rdx + 16 * 6], xmm6
movss [rdx + 16 * 7], xmm7
movss [rdx + 16 * 8], xmm8
movss [rdx + 16 * 9], xmm9
movss [rdx + 16 * 10], xmm10
movss [rdx + 16 * 11], xmm11
movss [rdx + 16 * 12], xmm12
movss [rdx + 16 * 13], xmm13
movss [rdx + 16 * 14], xmm14
movss [rdx + 16 * 15], xmm15

lea rdx, [rel .data]
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
movapd xmm10, [rdx + 16 * 10]
movapd xmm11, [rdx + 16 * 11]
movapd xmm12, [rdx + 16 * 12]
movapd xmm13, [rdx + 16 * 13]
movapd xmm14, [rdx + 16 * 14]
movapd xmm15, [rdx + 16 * 15]

mov rdx, 0xe0000000

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
movapd xmm10, [rdx + 16 * 10]
movapd xmm11, [rdx + 16 * 11]
movapd xmm12, [rdx + 16 * 12]
movapd xmm13, [rdx + 16 * 13]
movapd xmm14, [rdx + 16 * 14]
movapd xmm15, [rdx + 16 * 15]

hlt

align 16
; 512bytes of random data
.data:
dd 83.0999,69.50512,41.02678,13.05881,5.35242,21.9932,9.67383,5.32372,29.02872,66.50151,19.30764,91.3633,40.45086,50.96153,32.64489,23.97574,90.64316,24.22547,98.9394,91.21715,90.80143,99.48407,64.97245,74.39838,35.22761,25.35321,5.8732,90.19956,33.03133,52.02952,58.38554,10.17531,47.84703,84.04831,90.02965,65.81329,96.27991,6.64479,25.58971,95.00694,88.1929,37.16964,49.52602,10.27223,77.70605,20.21439,9.8056,41.29389,15.4071,57.54286,9.61117,55.54302,52.90745,4.88086,72.52882,3.0201,56.55091,71.22749,61.84736,88.74295,47.72641,24.17404,33.70564,96.71303


