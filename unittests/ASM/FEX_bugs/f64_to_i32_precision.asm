%ifdef CONFIG
{
  "HostFeatures": ["AVX"],
  "Env": { "FEX_HOSTFEATURES": "disablesve,disablefrintts" },
  "RegData": {
    "XMM0": ["0x297e3ce700000000", "0x0000000000000000", "0x0000000000000000", "0x0000000000000000"]
  }
}
%endif

; The non-SVE path in Vector_F64ToI32 (JIT/ConversionOps.cpp) had a precision issue:
; Used to narrow the 64-bit float down to a 32-bit float before
; converting to an integer, rather than converting directly to a 64-bit integer and
; narrowing that to a 32-bit float.
lea rdx, [rel .data]
vcvttpd2dq xmm0, oword [rdx]
hlt

align 16
.data:
dq 0x8d482dd627d581f1, 0x41c4bf1e7380f30a
