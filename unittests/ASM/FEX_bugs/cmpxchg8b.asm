%ifdef CONFIG
{
  "RegData": {
    "R13": "0x20",
    "R14": "0x0000000000010c74",
    "R15": "1"
  }
}
%endif

; FEX-Emu had a bug where cmpxchg8b/cmpxchg16b wasn't setting z flag correctly on flagm supporting CPUs.
; This runs through all the configurations to both ensure that the z flag is set correctly, and that the other flags aren't affected.
; There is an additional cmpxchg16b test in another file.
global temp_data

%define FULL_FLAGS ((1 << 0) | (1 << 2) | (1 << 4) | (1 << 6) | (1 << 7))
%define EMPTY_FLAGS

%define DATA_LOW 0x4142434445464748
%define DATA_HIGH 0x5152535455565758

%define DATA_INCORRECT_LOW 0x4142433445464748
%define DATA_INCORRECT_HIGH 0x5152533455565758

; Clobbers: RAX
%macro reset_data 0
  mov rax, DATA_LOW
  mov [rel temp_data], rax
  mov rax, DATA_HIGH
  mov [rel temp_data + 8], rax
%endmacro

; Clobbers: RAX
; Args: <immediate constant>
%macro reset_flag 1
  mov rax, %1
  push rax
  popfq
%endmacro

; Clobbers: RAX, RBX, RCX, RDX
; Compares incoming values against memory, and replaces
; Args: <64-bit immediate>
; Return: Compare result in ZF (ZF==1 == compare_success)
%macro cas_u32x2 1
  mov eax, (%1 & 0xFFFF_FFFF)
  mov edx, %1 >> 32

  mov ebx, 0
  mov ecx, 0
  cmpxchg8b [rel temp_data]
%endmacro

; Clobbers: RAX, RBX
; Compares flags with correct result
; Args: <jnz if ZF == 0, jz if ZF == 1>, <Expected other flags>
%macro check_flags 2
  pushfq
  pop rbx

  ; Remove IF and Reserved
  and rbx, ~(0x202)

  ; just compare ZF
  mov rax, rbx
  and rax, (1 << 6)
%if %1 == 0
  jz %%.no_error
%else
  jnz %%.no_error
%endif

  hlt
%%.no_error:

  ; compare full flags
  cmp rbx, %2

  je %%.second_no_error
  hlt

%%.second_no_error:
  ; Continue forward
%endmacro

; Args: <Compare Result>, <Expected flags>, <Data to compare>
%macro do_test 3
  reset_flag %2
  reset_data
  cas_u32x2 %3

  ; Check flag adding reserved for comparison
  check_flags %1,  %2
%endmacro

mov r15, 0
mov r14, 0
mov r13, 0
mov rsp, 0xe000_1000

; %define FULL_FLAGS ((1 << 0) | (1 << 2) | (1 << 4) | (1 << 6) | (1 << 7))
%assign eq_val 0

; EQ
%rep 2
  %assign flag_0 0
  %rep 2
    %assign flag_2 0
    %rep 2
      %assign flag_4 0
      %rep 2
        %assign flag_7 0
        %rep 2
          ; if eq_val == 0 then zf is expected to be zero, aka non-match
          %if eq_val == 0
            %assign data DATA_INCORRECT_LOW
          %else
            %assign data DATA_LOW
          %endif

          %assign expected_flags ((flag_0 << 0) | (flag_2 << 2) | (flag_4 << 4) | (flag_7 << 7) | (eq_val << 6))
          inc r13
          do_test eq_val, expected_flags, data
          %assign flag_7 flag_7+1
        %endrep
        %assign flag_4 flag_4+1
      %endrep
      %assign flag_2 flag_2+1
    %endrep
    %assign flag_0 flag_0+1
  %endrep

  %assign eq_val eq_val+1
%endrep

mov r15, 1
lea r14, [rel .end]
.end:
hlt

align 4096
temp_data:
dq 0
dq 0
