%ifdef CONFIG
{
  "RegData": {
      "RAX": "0x00000000",
      "RBX": "0x00000000",
      "RCX": "0x00000000",
      "RSI": "0x00000000"
  }
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

%macro min 3
  single_case min, %1, %2, %3
%endmacro

%macro max 3
  single_case max, %1, %2, %3
%endmacro

zero equ 0x00000000
negzero equ 0x80000000
qnan equ 0x7fc00000
snan equ 0x7f800001

cases:
  ; Basic identities
  min zero,    zero,    zero
  max zero,    zero,    zero
  min negzero, negzero, negzero
  max negzero, negzero, negzero
  min qnan,    qnan,    qnan
  max qnan,    qnan,    qnan

  ; "If the values being compared are both 0.0s (of either sign), the value in
  ; the second source operand is returned"
  min zero,    negzero, negzero
  max zero,    negzero, negzero
  min negzero, zero,    zero
  max negzero, zero,    zero

  ; "If only one value is a NaN (SNaN or QNaN) for this instruction, the second
  ; source operand, either a NaN or a valid floating-point value, is written to
  ; the result"
  min zero,    qnan,    qnan
  min negzero, qnan,    qnan
  min qnan,    zero,    zero
  min qnan,    negzero, negzero

  max zero,    qnan,    qnan
  max negzero, qnan,    qnan
  max qnan,    zero,    zero
  max qnan,    negzero, negzero

  min zero,    snan,    snan
  min negzero, snan,    snan
  min snan,    zero,    zero
  min snan,    negzero, negzero

  max zero,    snan,    snan
  max negzero, snan,    snan
  max snan,    zero,    zero
  max snan,    negzero, negzero

  ; "If a value in the second operand is an SNaN, that SNaN is returned
  ; unchanged to the destination (that is, a QNaN version of the SNaN is not
  ; returned)."
  min qnan, snan, snan
  min snan, snan, snan

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
