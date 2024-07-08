%ifdef CONFIG
{
  "HostFeatures": ["AVX"],
  "RegData": {
    "XMM0":  ["0x4142434445464748", "0", "0", "0"],
    "XMM1":  ["0x4142434445464748", "0", "0x7172737475767778", "0x8182838485868788"],
    "XMM2":  ["0x0000000041424344", "0", "0", "0"],
    "XMM3":  ["0x0000000041424344", "0", "0x7172737475767778", "0x8182838485868788"]
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

; FEX-Emu had a bug where vmovq was loading 128-bits worth of data instead of 64-bits.
; This ensures that {v,}mov{d,q} all load the correct amount of data through a test that will fault if it loads too much.

; Address at the last eight bytes
mov rax, 0x100000000 + 4096-8

; Address at the last 4 bytes
mov rbx, 0x100000000 + 4096-4

mov rcx, 0x4142434445464748

; Store data using GPR
mov [rax], rcx

; Setup vector with data
vmovaps ymm0, [rel .data]
vmovaps ymm1, [rel .data]
vmovaps ymm2, [rel .data]
vmovaps ymm3, [rel .data]

; 64-bit tests

; Load with vmovq to ensure we don't try loading too much data
vmovq xmm0, qword [rax]

; Also test SSE2 version
movq xmm1, qword [rax]

; Also test MOVQ stores
vmovq qword [rax], xmm0

; Also test SSE2 version
movq qword [rax], xmm1

; 32-bit tests
; Load with vmovq to ensure we don't try loading too much data
vmovd xmm2, dword [rbx]

; Also test SSE2 version
movd xmm3, dword [rbx]

; Also test MOVD stores
vmovd dword [rbx], xmm2

; Also test SSE2 version
movd dword [rbx], xmm3

hlt

align 32
.data:
dq 0x5152535455565758, 0x6162636465666768
dq 0x7172737475767778, 0x8182838485868788
