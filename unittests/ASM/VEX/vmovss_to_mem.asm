%ifdef CONFIG
{
  "HostFeatures": ["AVX"],
  "RegData": {
    "XMM0":  ["0x0000000042A63326", "0x0000000000000000", "0x0000000000000000", "0x0000000000000000"],
    "XMM1":  ["0x0000000040AB4706", "0x0000000000000000", "0x0000000000000000", "0x0000000000000000"],
    "XMM2":  ["0x0000000041E83AD2", "0x0000000000000000", "0x0000000000000000", "0x0000000000000000"],
    "XMM3":  ["0x000000004221CDAE", "0x0000000000000000", "0x0000000000000000", "0x0000000000000000"],
    "XMM4":  ["0x0000000042B5494C", "0x0000000000000000", "0x0000000000000000", "0x0000000000000000"],
    "XMM5":  ["0x0000000042B59A55", "0x0000000000000000", "0x0000000000000000", "0x0000000000000000"],
    "XMM6":  ["0x00000000420CE913", "0x0000000000000000", "0x0000000000000000", "0x0000000000000000"],
    "XMM7":  ["0x0000000042042015", "0x0000000000000000", "0x0000000000000000", "0x0000000000000000"],
    "XMM8":  ["0x00000000423F635C", "0x0000000000000000", "0x0000000000000000", "0x0000000000000000"],
    "XMM9":  ["0x0000000042C08F50", "0x0000000000000000", "0x0000000000000000", "0x0000000000000000"],
    "XMM10": ["0x0000000042B062C4", "0x0000000000000000", "0x0000000000000000", "0x0000000000000000"],
    "XMM11": ["0x00000000429B697F", "0x0000000000000000", "0x0000000000000000", "0x0000000000000000"],
    "XMM12": ["0x000000004176837B", "0x0000000000000000", "0x0000000000000000", "0x0000000000000000"],
    "XMM13": ["0x000000004253A13B", "0x0000000000000000", "0x0000000000000000", "0x0000000000000000"],
    "XMM14": ["0x0000000042623422", "0x0000000000000000", "0x0000000000000000", "0x0000000000000000"],
    "XMM15": ["0x00000000423EE7D8", "0x0000000000000000", "0x0000000000000000", "0x0000000000000000"]
  }
}
%endif

lea rdx, [rel .data]

vmovapd xmm0,  [rdx + 16 * 0]
vmovapd xmm1,  [rdx + 16 * 1]
vmovapd xmm2,  [rdx + 16 * 2]
vmovapd xmm3,  [rdx + 16 * 3]
vmovapd xmm4,  [rdx + 16 * 4]
vmovapd xmm5,  [rdx + 16 * 5]
vmovapd xmm6,  [rdx + 16 * 6]
vmovapd xmm7,  [rdx + 16 * 7]
vmovapd xmm8,  [rdx + 16 * 8]
vmovapd xmm9,  [rdx + 16 * 9]
vmovapd xmm10, [rdx + 16 * 10]
vmovapd xmm11, [rdx + 16 * 11]
vmovapd xmm12, [rdx + 16 * 12]
vmovapd xmm13, [rdx + 16 * 13]
vmovapd xmm14, [rdx + 16 * 14]
vmovapd xmm15, [rdx + 16 * 15]

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

vmovss [rdx + 16 * 0], xmm0
vmovss [rdx + 16 * 1], xmm1
vmovss [rdx + 16 * 2], xmm2
vmovss [rdx + 16 * 3], xmm3
vmovss [rdx + 16 * 4], xmm4
vmovss [rdx + 16 * 5], xmm5
vmovss [rdx + 16 * 6], xmm6
vmovss [rdx + 16 * 7], xmm7
vmovss [rdx + 16 * 8], xmm8
vmovss [rdx + 16 * 9], xmm9
vmovss [rdx + 16 * 10], xmm10
vmovss [rdx + 16 * 11], xmm11
vmovss [rdx + 16 * 12], xmm12
vmovss [rdx + 16 * 13], xmm13
vmovss [rdx + 16 * 14], xmm14
vmovss [rdx + 16 * 15], xmm15

lea rdx, [rel .data]
vmovapd xmm0, [rdx + 16 * 0]
vmovapd xmm1, [rdx + 16 * 1]
vmovapd xmm2, [rdx + 16 * 2]
vmovapd xmm3, [rdx + 16 * 3]
vmovapd xmm4, [rdx + 16 * 4]
vmovapd xmm5, [rdx + 16 * 5]
vmovapd xmm6, [rdx + 16 * 6]
vmovapd xmm7, [rdx + 16 * 7]
vmovapd xmm8, [rdx + 16 * 8]
vmovapd xmm9, [rdx + 16 * 9]
vmovapd xmm10, [rdx + 16 * 10]
vmovapd xmm11, [rdx + 16 * 11]
vmovapd xmm12, [rdx + 16 * 12]
vmovapd xmm13, [rdx + 16 * 13]
vmovapd xmm14, [rdx + 16 * 14]
vmovapd xmm15, [rdx + 16 * 15]

mov rdx, 0xe0000000

vmovapd xmm0, [rdx + 16 * 0]
vmovapd xmm1, [rdx + 16 * 1]
vmovapd xmm2, [rdx + 16 * 2]
vmovapd xmm3, [rdx + 16 * 3]
vmovapd xmm4, [rdx + 16 * 4]
vmovapd xmm5, [rdx + 16 * 5]
vmovapd xmm6, [rdx + 16 * 6]
vmovapd xmm7, [rdx + 16 * 7]
vmovapd xmm8, [rdx + 16 * 8]
vmovapd xmm9, [rdx + 16 * 9]
vmovapd xmm10, [rdx + 16 * 10]
vmovapd xmm11, [rdx + 16 * 11]
vmovapd xmm12, [rdx + 16 * 12]
vmovapd xmm13, [rdx + 16 * 13]
vmovapd xmm14, [rdx + 16 * 14]
vmovapd xmm15, [rdx + 16 * 15]

hlt

align 16
; 512bytes of random data
.data:
dd 83.0999 , 69.50512, 41.02678, 13.05881
dd 5.35242 , 21.9932 , 9.67383 , 5.32372
dd 29.02872, 66.50151, 19.30764, 91.3633
dd 40.45086, 50.96153, 32.64489, 23.97574
dd 90.64316, 24.22547, 98.9394 , 91.21715
dd 90.80143, 99.48407, 64.97245, 74.39838
dd 35.22761, 25.35321, 5.8732  , 90.19956
dd 33.03133, 52.02952, 58.38554, 10.17531
dd 47.84703, 84.04831, 90.02965, 65.81329
dd 96.27991, 6.64479 , 25.58971, 95.00694
dd 88.1929 , 37.16964, 49.52602, 10.27223
dd 77.70605, 20.21439, 9.8056  , 41.29389
dd 15.4071 , 57.54286, 9.61117 , 55.54302
dd 52.90745, 4.88086 , 72.52882, 3.0201
dd 56.55091, 71.22749, 61.84736, 88.74295
dd 47.72641, 24.17404, 33.70564, 96.71303
