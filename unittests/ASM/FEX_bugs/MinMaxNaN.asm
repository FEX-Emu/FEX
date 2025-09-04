%ifdef CONFIG
{
  "RegData": {
      "RAX": "0x00000000",
      "RBX": "0x00000000",
      "RCX": "0x00000000",
      "RSI": "0x00000000"
  },
  "HostFeatures": ["AVX"]
}
%endif

%macro single_case 4
  ; Load sources
  mov eax, %2
  mov ebx, %3
  movd xmm0, eax
  movd xmm1, ebx

  ; Calculate scalar min/max
  %1ss xmm0, xmm1

  ; Check result
  movd ecx, xmm0
  cmp ecx, %4
  jne fexi_fexi_im_so_broken
  mov ecx, 0

  ; Now try the SSE vector
  %1ps xmm0, xmm1
  movd ecx, xmm0
  cmp ecx, %4
  jne fexi_fexi_im_so_broken
  mov ecx, 0

  ; And the AVX-128 version
  v%1ps xmm2, xmm0, xmm1
  movd ecx, xmm2
  cmp ecx, %4
  jne fexi_fexi_im_so_broken
  mov ecx, 0

  ; And the AVX-256 version
  v%1ps ymm2, ymm0, ymm1
  movd ecx, xmm2
  cmp ecx, %4
  jne fexi_fexi_im_so_broken

%endmacro

%macro case_d 4
  ; Load sources
  mov rax, %2
  mov rbx, %3
  movq xmm0, rax
  movq xmm1, rbx

  ; Calculate scalar min/max
  %1sd xmm0, xmm1

  ; Check result
  movq rcx, xmm0
  mov rdx, %4
  cmp rcx, rdx
  jne fexi_fexi_im_so_broken
  mov rcx, 0

  ; Now try the SSE vector
  %1pd xmm0, xmm1
  movq rcx, xmm0
  mov rdx, %4
  cmp rcx, rdx
  jne fexi_fexi_im_so_broken
  mov rcx, 0

  ; And the AVX-128 version
  v%1pd xmm2, xmm0, xmm1
  movq rcx, xmm2
  mov rdx, %4
  cmp rcx, rdx
  jne fexi_fexi_im_so_broken
  mov rcx, 0

  ; And the AVX-256 version
  v%1pd ymm2, ymm0, ymm1
  movq rcx, xmm2
  mov rdx, %4
  cmp rcx, rdx
  jne fexi_fexi_im_so_broken
%endmacro

%macro min_s 3
  single_case min, %1, %2, %3
%endmacro

%macro max_s 3
  single_case max, %1, %2, %3
%endmacro

%macro min_d 3
  case_d min, %1, %2, %3
%endmacro

%macro max_d 3
  case_d max, %1, %2, %3
%endmacro

zero_s equ 0x00000000
negzero_s equ 0x80000000
qnan_s equ 0x7fc00000
snan_s equ 0x7f800001

zero_d equ 0x0000_0000_0000_0000
negzero_d equ 0x8000_0000_0000_0000
qnan_d equ 0x7ff8_0000_0000_0000
snan_d equ 0x7ff0_0000_0000_0001

%macro cases 1
  ; Basic identities
  min%1 zero%1,    zero%1,    zero%1
  max%1 zero%1,    zero%1,    zero%1
  min%1 negzero%1, negzero%1, negzero%1
  max%1 negzero%1, negzero%1, negzero%1
  min%1 qnan%1,    qnan%1,    qnan%1
  max%1 qnan%1,    qnan%1,    qnan%1

  ; "If the values being compared are both 0.0s (of either sign), the value in
  ; the second source operand is returned"
  min%1 zero%1,    negzero%1, negzero%1
  max%1 zero%1,    negzero%1, negzero%1
  min%1 negzero%1, zero%1,    zero%1
  max%1 negzero%1, zero%1,    zero%1

  ; "If only one value is a NaN (SNaN or QNaN) for this instruction, the second
  ; source operand, either a NaN or a valid floating-point value, is written to
  ; the result"
  min%1 zero%1,    qnan%1,    qnan%1
  min%1 negzero%1, qnan%1,    qnan%1
  min%1 qnan%1,    zero%1,    zero%1
  min%1 qnan%1,    negzero%1, negzero%1

  max%1 zero%1,    qnan%1,    qnan%1
  max%1 negzero%1, qnan%1,    qnan%1
  max%1 qnan%1,    zero%1,    zero%1
  max%1 qnan%1,    negzero%1, negzero%1

  min%1 zero%1,    snan%1,    snan%1
  min%1 negzero%1, snan%1,    snan%1
  min%1 snan%1,    zero%1,    zero%1
  min%1 snan%1,    negzero%1, negzero%1

  max%1 zero%1,    snan%1,    snan%1
  max%1 negzero%1, snan%1,    snan%1
  max%1 snan%1,    zero%1,    zero%1
  max%1 snan%1,    negzero%1, negzero%1

  ; "If a value in the second operand is an SNaN, that SNaN is returned
  ; unchanged to the destination (that is, a QNaN version of the SNaN is not
  ; returned)."
  min%1 qnan%1, snan%1, snan%1
  min%1 snan%1, snan%1, snan%1
%endmacro

single_cases:
  cases _s

cases_double:
  cases _d

success:
  mov rax, 0
  mov rbx, 0
  mov rcx, 0
  mov rsi, 0
  hlt

fexi_fexi_im_so_broken:
  ; Leave rax/rbx/rcx as-is for inspection
  mov rsi, 0xdeadbeef
  hlt
