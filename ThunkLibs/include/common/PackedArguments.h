#pragma once

template<typename Result, typename... Args>
struct PackedArguments;

template<typename R, typename A0>
struct PackedArguments<R, A0> { A0 a0; R rv; };
template<typename R, typename A0, typename A1>
struct PackedArguments<R, A0, A1> { A0 a0; A1 a1; R rv; };
template<typename R, typename A0, typename A1, typename A2>
struct PackedArguments<R, A0, A1, A2> { A0 a0; A1 a1; A2 a2; R rv; };
template<typename R, typename A0, typename A1, typename A2, typename A3>
struct PackedArguments<R, A0, A1, A2, A3> { A0 a0; A1 a1; A2 a2; A3 a3; R rv; };
template<typename R, typename A0, typename A1, typename A2, typename A3, typename A4>
struct PackedArguments<R, A0, A1, A2, A3, A4> { A0 a0; A1 a1; A2 a2; A3 a3; A4 a4; R rv; };
template<typename R, typename A0, typename A1, typename A2, typename A3, typename A4, typename A5>
struct PackedArguments<R, A0, A1, A2, A3, A4, A5> { A0 a0; A1 a1; A2 a2; A3 a3; A4 a4; A5 a5; R rv; };
template<typename R, typename A0, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6>
struct PackedArguments<R, A0, A1, A2, A3, A4, A5, A6> { A0 a0; A1 a1; A2 a2; A3 a3; A4 a4; A5 a5; A6 a6; R rv; };
template<typename R, typename A0, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7>
struct PackedArguments<R, A0, A1, A2, A3, A4, A5, A6, A7> { A0 a0; A1 a1; A2 a2; A3 a3; A4 a4; A5 a5; A6 a6; A7 a7; R rv; };
template<typename R, typename A0, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8>
struct PackedArguments<R, A0, A1, A2, A3, A4, A5, A6, A7, A8> { A0 a0; A1 a1; A2 a2; A3 a3; A4 a4; A5 a5; A6 a6; A7 a7; A8 a8; R rv; };
template<typename R, typename A0, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8, typename A9>
struct PackedArguments<R, A0, A1, A2, A3, A4, A5, A6, A7, A8, A9> { A0 a0; A1 a1; A2 a2; A3 a3; A4 a4; A5 a5; A6 a6; A7 a7; A8 a8; A9 a9; R rv; };
template<typename R, typename A0, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8, typename A9, typename A10>
struct PackedArguments<R, A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10> { A0 a0; A1 a1; A2 a2; A3 a3; A4 a4; A5 a5; A6 a6; A7 a7; A8 a8; A9 a9; A10 a10; R rv; };
template<typename R, typename A0, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8, typename A9, typename A10, typename A11>
struct PackedArguments<R, A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11> { A0 a0; A1 a1; A2 a2; A3 a3; A4 a4; A5 a5; A6 a6; A7 a7; A8 a8; A9 a9; A10 a10; A11 a11; R rv; };
template<typename R, typename A0, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8, typename A9, typename A10, typename A11, typename A12>
struct PackedArguments<R, A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12> { A0 a0; A1 a1; A2 a2; A3 a3; A4 a4; A5 a5; A6 a6; A7 a7; A8 a8; A9 a9; A10 a10; A11 a11; A12 a12; R rv; };

template<typename R, typename A0, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8, typename A9, typename A10, typename A11, typename A12, typename A13>
struct PackedArguments<R, A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13> { A0 a0; A1 a1; A2 a2; A3 a3; A4 a4; A5 a5; A6 a6; A7 a7; A8 a8; A9 a9; A10 a10; A11 a11; A12 a12; A13 a13; R rv; };

template<typename R, typename A0, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8, typename A9,
         typename A10, typename A11, typename A12, typename A13, typename A14, typename A15, typename A16, typename A17, typename A18, typename A19,
         typename A20, typename A21, typename A22, typename A23>
struct PackedArguments<R, A0, A1, A2, A3, A4, A5, A6, A7, A8, A9,
                       A10, A11, A12, A13, A14, A15, A16, A17, A18, A19,
                       A20, A21, A22, A23> {
    A0 a0; A1 a1; A2 a2; A3 a3; A4 a4; A5 a5; A6 a6; A7 a7; A8 a8; A9 a9;
    A10 a10; A11 a11; A12 a12; A13 a13; A14 a14; A15 a15; A16 a16; A17 a17; A18 a18; A19 a19;
    A20 a20; A21 a21; A22 a22; A23 a23; R rv;
};

template<typename A0>
struct PackedArguments<void, A0> { A0 a0; };
template<typename A0, typename A1>
struct PackedArguments<void, A0, A1> { A0 a0; A1 a1; };
template<typename A0, typename A1, typename A2>
struct PackedArguments<void, A0, A1, A2> { A0 a0; A1 a1; A2 a2; };
template<typename A0, typename A1, typename A2, typename A3>
struct PackedArguments<void, A0, A1, A2, A3> { A0 a0; A1 a1; A2 a2; A3 a3; };
template<typename A0, typename A1, typename A2, typename A3, typename A4>
struct PackedArguments<void, A0, A1, A2, A3, A4> { A0 a0; A1 a1; A2 a2; A3 a3; A4 a4; };
template<typename A0, typename A1, typename A2, typename A3, typename A4, typename A5>
struct PackedArguments<void, A0, A1, A2, A3, A4, A5> { A0 a0; A1 a1; A2 a2; A3 a3; A4 a4; A5 a5; };
template<typename A0, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6>
struct PackedArguments<void, A0, A1, A2, A3, A4, A5, A6> { A0 a0; A1 a1; A2 a2; A3 a3; A4 a4; A5 a5; A6 a6; };
template<typename A0, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7>
struct PackedArguments<void, A0, A1, A2, A3, A4, A5, A6, A7> { A0 a0; A1 a1; A2 a2; A3 a3; A4 a4; A5 a5; A6 a6; A7 a7; };
template<typename A0, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8>
struct PackedArguments<void, A0, A1, A2, A3, A4, A5, A6, A7, A8> { A0 a0; A1 a1; A2 a2; A3 a3; A4 a4; A5 a5; A6 a6; A7 a7; A8 a8; };
template<typename A0, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8, typename A9>
struct PackedArguments<void, A0, A1, A2, A3, A4, A5, A6, A7, A8, A9> { A0 a0; A1 a1; A2 a2; A3 a3; A4 a4; A5 a5; A6 a6; A7 a7; A8 a8; A9 a9; };
template<typename A0, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8, typename A9, typename A10>
struct PackedArguments<void, A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10> { A0 a0; A1 a1; A2 a2; A3 a3; A4 a4; A5 a5; A6 a6; A7 a7; A8 a8; A9 a9; A10 a10; };
template<typename A0, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8, typename A9, typename A10, typename A11>
struct PackedArguments<void, A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11> { A0 a0; A1 a1; A2 a2; A3 a3; A4 a4; A5 a5; A6 a6; A7 a7; A8 a8; A9 a9; A10 a10; A11 a11; };
template<typename A0, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8, typename A9, typename A10, typename A11, typename A12>
struct PackedArguments<void, A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12> { A0 a0; A1 a1; A2 a2; A3 a3; A4 a4; A5 a5; A6 a6; A7 a7; A8 a8; A9 a9; A10 a10; A11 a11; A12 a12; };
template<typename A0, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8, typename A9, typename A10, typename A11, typename A12, typename A13>
struct PackedArguments<void, A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13> { A0 a0; A1 a1; A2 a2; A3 a3; A4 a4; A5 a5; A6 a6; A7 a7; A8 a8; A9 a9; A10 a10; A11 a11; A12 a12; A13 a13; };
template<typename A0, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8, typename A9, typename A10, typename A11, typename A12, typename A13, typename A14>
struct PackedArguments<void, A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14> { A0 a0; A1 a1; A2 a2; A3 a3; A4 a4; A5 a5; A6 a6; A7 a7; A8 a8; A9 a9; A10 a10; A11 a11; A12 a12; A13 a13; A14 a14; };
template<typename A0, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8, typename A9, typename A10, typename A11, typename A12, typename A13, typename A14, typename A15>
struct PackedArguments<void, A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15> { A0 a0; A1 a1; A2 a2; A3 a3; A4 a4; A5 a5; A6 a6; A7 a7; A8 a8; A9 a9; A10 a10; A11 a11; A12 a12; A13 a13; A14 a14; A15 a15; };
template<typename A0, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8, typename A9, typename A10, typename A11, typename A12, typename A13, typename A14, typename A15, typename A16>
struct PackedArguments<void, A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16> { A0 a0; A1 a1; A2 a2; A3 a3; A4 a4; A5 a5; A6 a6; A7 a7; A8 a8; A9 a9; A10 a10; A11 a11; A12 a12; A13 a13; A14 a14; A15 a15; A16 a16; };
template<typename A0, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8, typename A9, typename A10, typename A11, typename A12, typename A13, typename A14, typename A15, typename A16, typename A17>
struct PackedArguments<void, A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16, A17> { A0 a0; A1 a1; A2 a2; A3 a3; A4 a4; A5 a5; A6 a6; A7 a7; A8 a8; A9 a9; A10 a10; A11 a11; A12 a12; A13 a13; A14 a14; A15 a15; A16 a16; A17 a17; };
