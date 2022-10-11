#pragma once

#include <cstdint>
#include <type_traits>

template<typename Result, typename... Args>
struct PackedArguments;

template<typename R>
struct PackedArguments<R> { R rv; };
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
template<typename R, typename A0, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8, typename A9, typename A10, typename A11, typename A12, typename A13, typename A14>
struct PackedArguments<R, A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14> { A0 a0; A1 a1; A2 a2; A3 a3; A4 a4; A5 a5; A6 a6; A7 a7; A8 a8; A9 a9; A10 a10; A11 a11; A12 a12; A13 a13; A14 a14; R rv; };

template<typename R, typename A0, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8, typename A9,
         typename A10, typename A11, typename A12, typename A13, typename A14, typename A15, typename A16, typename A17, typename A18, typename A19,
         typename A20, typename A21, typename A22>
struct PackedArguments<R, A0, A1, A2, A3, A4, A5, A6, A7, A8, A9,
                       A10, A11, A12, A13, A14, A15, A16, A17, A18, A19,
                       A20, A21, A22> {
    A0 a0; A1 a1; A2 a2; A3 a3; A4 a4; A5 a5; A6 a6; A7 a7; A8 a8; A9 a9;
    A10 a10; A11 a11; A12 a12; A13 a13; A14 a14; A15 a15; A16 a16; A17 a17; A18 a18; A19 a19;
    A20 a20; A21 a21; A22 a22; R rv;
};

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

template<>
struct PackedArguments<void> { };
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
template<typename A0, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8, typename A9, typename A10, typename A11, typename A12, typename A13, typename A14, typename A15, typename A16, typename A17, typename A18>
struct PackedArguments<void, A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16, A17, A18> { A0 a0; A1 a1; A2 a2; A3 a3; A4 a4; A5 a5; A6 a6; A7 a7; A8 a8; A9 a9; A10 a10; A11 a11; A12 a12; A13 a13; A14 a14; A15 a15; A16 a16; A17 a17; A18 a18; };

template<typename Result, typename... Args>
void Invoke(Result(*func)(Args...), PackedArguments<Result, Args...>& args) {
    constexpr auto NumArgs = sizeof...(Args);
    static_assert(NumArgs <= 19 || NumArgs == 24);
    if constexpr (std::is_void_v<Result>) {
        if constexpr (NumArgs == 0) {
            func();
        } else if constexpr (NumArgs == 1) {
            func(args.a0);
        } else if constexpr (NumArgs == 2) {
            func(args.a0, args.a1);
        } else if constexpr (NumArgs == 3) {
            func(args.a0, args.a1, args.a2);
        } else if constexpr (NumArgs == 4) {
            func(args.a0, args.a1, args.a2, args.a3);
        } else if constexpr (NumArgs == 5) {
            func(args.a0, args.a1, args.a2, args.a3, args.a4);
        } else if constexpr (NumArgs == 6) {
            func(args.a0, args.a1, args.a2, args.a3, args.a4, args.a5);
        } else if constexpr (NumArgs == 7) {
            func(args.a0, args.a1, args.a2, args.a3, args.a4, args.a5, args.a6);
        } else if constexpr (NumArgs == 8) {
            func(args.a0, args.a1, args.a2, args.a3, args.a4, args.a5, args.a6, args.a7);
        } else if constexpr (NumArgs == 9) {
            func(args.a0, args.a1, args.a2, args.a3, args.a4, args.a5, args.a6, args.a7, args.a8);
        } else if constexpr (NumArgs == 10) {
            func(args.a0, args.a1, args.a2, args.a3, args.a4, args.a5, args.a6, args.a7, args.a8, args.a9);
        } else if constexpr (NumArgs == 11) {
            func(args.a0, args.a1, args.a2, args.a3, args.a4, args.a5, args.a6, args.a7, args.a8, args.a9, args.a10);
        } else if constexpr (NumArgs == 12) {
            func(args.a0, args.a1, args.a2, args.a3, args.a4, args.a5, args.a6, args.a7, args.a8, args.a9, args.a10, args.a11);
        } else if constexpr (NumArgs == 13) {
            func(args.a0, args.a1, args.a2, args.a3, args.a4, args.a5, args.a6, args.a7, args.a8, args.a9, args.a10, args.a11, args.a12);
        } else if constexpr (NumArgs == 14) {
            func(args.a0, args.a1, args.a2, args.a3, args.a4, args.a5, args.a6, args.a7, args.a8, args.a9, args.a10, args.a11, args.a12, args.a13);
        } else if constexpr (NumArgs == 15) {
            func(args.a0, args.a1, args.a2, args.a3, args.a4, args.a5, args.a6, args.a7, args.a8, args.a9, args.a10, args.a11, args.a12, args.a13, args.a14);
        } else if constexpr (NumArgs == 16) {
            func(args.a0, args.a1, args.a2, args.a3, args.a4, args.a5, args.a6, args.a7, args.a8, args.a9, args.a10, args.a11, args.a12, args.a13, args.a14, args.a15);
        } else if constexpr (NumArgs == 17) {
            func(args.a0, args.a1, args.a2, args.a3, args.a4, args.a5, args.a6, args.a7, args.a8, args.a9, args.a10, args.a11, args.a12, args.a13, args.a14, args.a15, args.a16);
        } else if constexpr (NumArgs == 18) {
            func(args.a0, args.a1, args.a2, args.a3, args.a4, args.a5, args.a6, args.a7, args.a8, args.a9, args.a10, args.a11, args.a12, args.a13, args.a14, args.a15, args.a16, args.a17);
        } else if constexpr (NumArgs == 19) {
            func(args.a0, args.a1, args.a2, args.a3, args.a4, args.a5, args.a6, args.a7, args.a8, args.a9, args.a10, args.a11, args.a12, args.a13, args.a14, args.a15, args.a16, args.a17, args.a18);
        } else if constexpr (NumArgs == 24) {
            func(args.a0, args.a1, args.a2, args.a3, args.a4, args.a5, args.a6, args.a7, args.a8, args.a9, args.a10, args.a11, args.a12, args.a13, args.a14, args.a15, args.a16, args.a17, args.a18, args.a19, args.a20, args.a21, args.a22, args.a23);
        }
    } else {
        if constexpr (NumArgs == 0) {
            args.rv = func();
        } else if constexpr (NumArgs == 1) {
            args.rv = func(args.a0);
        } else if constexpr (NumArgs == 2) {
            args.rv = func(args.a0, args.a1);
        } else if constexpr (NumArgs == 3) {
            args.rv = func(args.a0, args.a1, args.a2);
        } else if constexpr (NumArgs == 4) {
            args.rv = func(args.a0, args.a1, args.a2, args.a3);
        } else if constexpr (NumArgs == 5) {
            args.rv = func(args.a0, args.a1, args.a2, args.a3, args.a4);
        } else if constexpr (NumArgs == 6) {
            args.rv = func(args.a0, args.a1, args.a2, args.a3, args.a4, args.a5);
        } else if constexpr (NumArgs == 7) {
            args.rv = func(args.a0, args.a1, args.a2, args.a3, args.a4, args.a5, args.a6);
        } else if constexpr (NumArgs == 8) {
            args.rv = func(args.a0, args.a1, args.a2, args.a3, args.a4, args.a5, args.a6, args.a7);
        } else if constexpr (NumArgs == 9) {
            args.rv = func(args.a0, args.a1, args.a2, args.a3, args.a4, args.a5, args.a6, args.a7, args.a8);
        } else if constexpr (NumArgs == 10) {
            args.rv = func(args.a0, args.a1, args.a2, args.a3, args.a4, args.a5, args.a6, args.a7, args.a8, args.a9);
        } else if constexpr (NumArgs == 11) {
            args.rv = func(args.a0, args.a1, args.a2, args.a3, args.a4, args.a5, args.a6, args.a7, args.a8, args.a9, args.a10);
        } else if constexpr (NumArgs == 12) {
            args.rv = func(args.a0, args.a1, args.a2, args.a3, args.a4, args.a5, args.a6, args.a7, args.a8, args.a9, args.a10, args.a11);
        } else if constexpr (NumArgs == 13) {
            args.rv = func(args.a0, args.a1, args.a2, args.a3, args.a4, args.a5, args.a6, args.a7, args.a8, args.a9, args.a10, args.a11, args.a12);
        } else if constexpr (NumArgs == 14) {
            args.rv = func(args.a0, args.a1, args.a2, args.a3, args.a4, args.a5, args.a6, args.a7, args.a8, args.a9, args.a10, args.a11, args.a12, args.a13);
        } else if constexpr (NumArgs == 15) {
            args.rv = func(args.a0, args.a1, args.a2, args.a3, args.a4, args.a5, args.a6, args.a7, args.a8, args.a9, args.a10, args.a11, args.a12, args.a13, args.a14);
        } else if constexpr (NumArgs == 16) {
            args.rv = func(args.a0, args.a1, args.a2, args.a3, args.a4, args.a5, args.a6, args.a7, args.a8, args.a9, args.a10, args.a11, args.a12, args.a13, args.a14, args.a15);
        } else if constexpr (NumArgs == 17) {
            args.rv = func(args.a0, args.a1, args.a2, args.a3, args.a4, args.a5, args.a6, args.a7, args.a8, args.a9, args.a10, args.a11, args.a12, args.a13, args.a14, args.a15, args.a16);
        } else if constexpr (NumArgs == 18) {
            args.rv = func(args.a0, args.a1, args.a2, args.a3, args.a4, args.a5, args.a6, args.a7, args.a8, args.a9, args.a10, args.a11, args.a12, args.a13, args.a14, args.a15, args.a16, args.a17);
        } else if constexpr (NumArgs == 19) {
            args.rv = func(args.a0, args.a1, args.a2, args.a3, args.a4, args.a5, args.a6, args.a7, args.a8, args.a9, args.a10, args.a11, args.a12, args.a13, args.a14, args.a15, args.a16, args.a17, args.a18);
        } else if constexpr (NumArgs == 24) {
            args.rv = func(args.a0, args.a1, args.a2, args.a3, args.a4, args.a5, args.a6, args.a7, args.a8, args.a9, args.a10, args.a11, args.a12, args.a13, args.a14, args.a15, args.a16, args.a17, args.a18, args.a19, args.a20, args.a21, args.a22, args.a23);
        }
    }
}
