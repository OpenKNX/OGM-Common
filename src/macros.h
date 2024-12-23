/**
 * OpenKNX - macros.h
 * 
 * Macros contains a collection of useful macros for various purposes, 
 * including bit manipulation, stringification, conditional compilation, and more.
 * 
 * Based on Marlin 3D Printer Firmware - Copyright (c) 2020 MarlinFirmware 
 * 
 * The following categories of macros are included: 
 * - Bit Manipulation Macros:
 * - Stringification Macros:
 * - Inline and No-inline Macros:
 * - Conditional Compilation Macros:
 * - Array and List Macros:
 * - Mathematical Macros:
 * - Helper Macros:
 * - Error Macros:
 * ...
 *
 */
#pragma once

#ifndef __has_include
  #define __has_include(...) 1
#endif

#define _FORCE_INLINE_ __attribute__((__always_inline__)) __inline__
#define  FORCE_INLINE  __attribute__((always_inline)) inline
#define NO_INLINE      __attribute__((noinline))
#define _UNUSED      __attribute__((unused))

#define IS_CONSTEXPR(...) __builtin_constant_p(__VA_ARGS__) // Only valid solution with C++14. Should use std::is_constant_evaluated() in C++20 instead

#ifndef UNUSED
  #define UNUSED(x) ((void)(x)) // Use UNUSED(x) to avoid unused variable warnings. Example: UNUSED(x); --> x is not used
#endif

// Macros to make a string from a macro
#define STRINGIFY_(M) #M // Use STRINGIFY_(M) to convert a macro to a string. Example: STRINGIFY_(M) --> "M"
#define STRINGIFY(M) STRINGIFY_(M)

#define A(CODE) " " CODE "\n\t" // Example: A("LDR R0, [R1]") --> " LDR R0, [R1]\n\t"
#define L(CODE) CODE ":\n\t"  // Example: L("loop") --> "loop:\n\t"

// Macros for bit masks
#undef _BV
#define _BV(b) (1 << (b)) // Bit value
#define TEST(n,b) (!!((n)&_BV(b))) // Test bit
#define SET_BIT_TO(N,B,TF) do{ if (TF) SBI(N,B); else CBI(N,B); }while(0) // Set bit to true or false
#ifndef SBI
  #define SBI(A,B) (A |= _BV(B)) // Set bit
#endif
#ifndef CBI
  #define CBI(A,B) (A &= ~_BV(B)) // Clear bit
#endif
#define TBI(N,B) (N ^= _BV(B)) // Toggle bit
#define _BV32(b) (1UL << (b)) // 32-bit bit value
#define TEST32(n,b) !!((n)&_BV32(b)) // Test 32-bit bit
#define SBI32(n,b) (n |= _BV32(b)) // Set 32-bit bit
#define CBI32(n,b) (n &= ~_BV32(b)) // Clear 32-bit bit
#define TBI32(N,B) (N ^= _BV32(B)) // Toggle 32-bit bit

// Hilfs-Makro zur Verkettung ohne Klammern
#define _CONCAT(a, b) a ## b
#define CONCAT(a, b) _CONCAT(a, b)

// Makros zur Verkettung der Bedingungen
#define _DO_1(W,C,A)        (_CONCAT(_##W##_1, A)) // Use _DO_1(W,C,A) to chain up to 40 conditions. Example: _DO_1(W,C,A) --> (_##W##_1(A))
#define _DO_2(W,C,A,B)      (_CONCAT(_##W##_1, A) C _CONCAT(_##W##_1, B))
#define _DO_3(W,C,A, V...)  (_CONCAT(_##W##_1, A) C _DO_2(W,C,V))
#define _DO_4(W,C,A, V...)  (_CONCAT(_##W##_1, A) C _DO_3(W,C,V))
#define _DO_5(W,C,A, V...)  (_CONCAT(_##W##_1, A) C _DO_4(W,C,V))
#define _DO_6(W,C,A, V...)  (_CONCAT(_##W##_1, A) C _DO_5(W,C,V))
#define _DO_7(W,C,A, V...)  (_CONCAT(_##W##_1, A) C _DO_6(W,C,V))
#define _DO_8(W,C,A, V...)  (_CONCAT(_##W##_1, A) C _DO_7(W,C,V))
#define _DO_9(W,C,A, V...)  (_CONCAT(_##W##_1, A) C _DO_8(W,C,V))
#define _DO_10(W,C,A,V...)  (_CONCAT(_##W##_1, A) C _DO_9(W,C,V))
#define _DO_11(W,C,A,V...)  (_CONCAT(_##W##_1, A) C _DO_10(W,C,V))
#define _DO_12(W,C,A,V...)  (_CONCAT(_##W##_1, A) C _DO_11(W,C,V))
#define _DO_13(W,C,A,V...)  (_CONCAT(_##W##_1, A) C _DO_12(W,C,V))
#define _DO_14(W,C,A,V...)  (_CONCAT(_##W##_1, A) C _DO_13(W,C,V))
#define _DO_15(W,C,A,V...)  (_CONCAT(_##W##_1, A) C _DO_14(W,C,V))
#define _DO_16(W,C,A,V...)  (_CONCAT(_##W##_1, A) C _DO_15(W,C,V))
#define _DO_17(W,C,A,V...)  (_CONCAT(_##W##_1, A) C _DO_16(W,C,V))
#define _DO_18(W,C,A,V...)  (_CONCAT(_##W##_1, A) C _DO_17(W,C,V))
#define _DO_19(W,C,A,V...)  (_CONCAT(_##W##_1, A) C _DO_18(W,C,V))
#define _DO_20(W,C,A,V...)  (_CONCAT(_##W##_1, A) C _DO_19(W,C,V))
#define _DO_21(W,C,A,V...)  (_CONCAT(_##W##_1, A) C _DO_20(W,C,V))
#define _DO_22(W,C,A,V...)  (_CONCAT(_##W##_1, A) C _DO_21(W,C,V))
#define _DO_23(W,C,A,V...)  (_CONCAT(_##W##_1, A) C _DO_22(W,C,V))
#define _DO_24(W,C,A,V...)  (_CONCAT(_##W##_1, A) C _DO_23(W,C,V))
#define _DO_25(W,C,A,V...)  (_CONCAT(_##W##_1, A) C _DO_24(W,C,V))
#define _DO_26(W,C,A,V...)  (_CONCAT(_##W##_1, A) C _DO_25(W,C,V))
#define _DO_27(W,C,A,V...)  (_CONCAT(_##W##_1, A) C _DO_26(W,C,V))
#define _DO_28(W,C,A,V...)  (_CONCAT(_##W##_1, A) C _DO_27(W,C,V))
#define _DO_29(W,C,A,V...)  (_CONCAT(_##W##_1, A) C _DO_28(W,C,V))
#define _DO_30(W,C,A,V...)  (_CONCAT(_##W##_1, A) C _DO_29(W,C,V))
#define _DO_31(W,C,A,V...)  (_CONCAT(_##W##_1, A) C _DO_30(W,C,V))
#define _DO_32(W,C,A,V...)  (_CONCAT(_##W##_1, A) C _DO_31(W,C,V))
#define _DO_33(W,C,A,V...)  (_CONCAT(_##W##_1, A) C _DO_32(W,C,V))
#define _DO_34(W,C,A,V...)  (_CONCAT(_##W##_1, A) C _DO_33(W,C,V))
#define _DO_35(W,C,A,V...)  (_CONCAT(_##W##_1, A) C _DO_34(W,C,V))
#define _DO_36(W,C,A,V...)  (_CONCAT(_##W##_1, A) C _DO_35(W,C,V))
#define _DO_37(W,C,A,V...)  (_CONCAT(_##W##_1, A) C _DO_36(W,C,V))
#define _DO_38(W,C,A,V...)  (_CONCAT(_##W##_1, A) C _DO_37(W,C,V))
#define _DO_39(W,C,A,V...)  (_CONCAT(_##W##_1, A) C _DO_38(W,C,V))
#define _DO_40(W,C,A,V...)  (_CONCAT(_##W##_1, A) C _DO_39(W,C,V))

// Dynamische Auswahl des richtigen Makros basierend auf der Argumentanzahl
#define __DO_N(W,C,N,V...)  _DO_##N(W,C,V)
#define _DO_N(W,C,N,V...)   __DO_N(W,C,N,V)
#define DO(W,C,V...)        (_DO_N(W,C,NUM_ARGS(V),V))

// Concatenate symbol names, ohne oder mit Vor-Expansion
#define _CAT(a,b) a ## b
#define CAT(a,b) _CAT(a,b)

// Makros zur Schaltoptionenprüfung
#define _ENA_1(O)           _ISENA(CAT(_IS,CAT(ENA_, O))) // Überprüfung der Aktivierung
#define _DIS_1(O)           NOT(_ENA_1(O))  // Überprüfung der Deaktivierung
#define ENABLED(V...)       DO(ENA,&&,V)  // Prüft, ob alle Optionen aktiviert sind
#define DISABLED(V...)      DO(DIS,&&,V)  // Prüft, ob alle Optionen deaktiviert sind
#define ANY(V...)           !DISABLED(V)  // Prüft, ob eine Option aktiviert ist
#define ALL(V...)           ENABLED(V) // Use ALL(V) to evaluate simple option switches. I.e. if all of the options are enabled
#define NONE(V...)          DISABLED(V) // Use NONE(V) to evaluate simple option switches. I.e. if none of the options are enabled
#define COUNT_ENABLED(V...) DO(ENA,+,V) // Use COUNT_ENABLED(V) to evaluate simple option switches. Example: COUNT_ENABLED(V) --> DO(ENA,+,V)
#define MANY(V...)          (COUNT_ENABLED(V) > 1) // Use MANY(V) to evaluate simple option switches. Example: MANY(V) --> (COUNT_ENABLED(V) > 1)

// Ternary pre-compiler macros conceal non-emitted content from the compiler
#define TERN(O,A,B)         _TERN(_ENA_1(O),B,A)    // OPTION ? 'A' : 'B' // Use TERN(O,A,B) to conceal non-emitted content from the compiler. Example: TERN(O,A,B) --> _TERN(_ENA_1(O),B,A)
#define TERN0(O,A)          _TERN(_ENA_1(O),0,A)    // OPTION ? 'A' : '0'
#define TERN1(O,A)          _TERN(_ENA_1(O),1,A)    // OPTION ? 'A' : '1'
#define TERN_(O,A)          _TERN(_ENA_1(O),,A)     // OPTION ? 'A' : '<nul>'
#define _TERN(E,V...)       __TERN(_CAT(T_,E),V)    // Prepend 'T_' to get 'T_0' or 'T_1'
#define __TERN(T,V...)      ___TERN(_CAT(_NO,T),V)  // Prepend '_NO' to get '_NOT_0' or '_NOT_1'
#define ___TERN(P,V...)     THIRD(P,V)              // If first argument has a comma, A. Else B.


// Macros to conditionally emit array items and function arguments
#define _OPTITEM(A...)      A,
#define OPTITEM(O,A...)     TERN_(O,DEFER4(_OPTITEM)(A))
#define _OPTARG(A...)       , A
#define OPTARG(O,A...)      TERN_(O,DEFER4(_OPTARG)(A))
#define _OPTCODE(A)         A;
#define OPTCODE(O,A)        TERN_(O,DEFER4(_OPTCODE)(A))

// Macros to avoid operations that aren't always optimized away (e.g., 'f + 0.0' and 'f * 1.0').
// Compiler flags -fno-signed-zeros -ffinite-math-only also cover 'f * 1.0', 'f - f', etc.
#define PLUS_TERN0(O,A)     _TERN(_ENA_1(O),,+ (A)) // OPTION ? '+ (A)' : '<nul>'
#define MINUS_TERN0(O,A)    _TERN(_ENA_1(O),,- (A)) // OPTION ? '- (A)' : '<nul>'
#define MUL_TERN1(O,A)      _TERN(_ENA_1(O),,* (A)) // OPTION ? '* (A)' : '<nul>'
#define DIV_TERN1(O,A)      _TERN(_ENA_1(O),,/ (A)) // OPTION ? '/ (A)' : '<nul>'
#define SUM_TERN(O,B,A)     ((B) PLUS_TERN0(O,A))   // ((B) (OPTION ? '+ (A)' : '<nul>'))
#define DIFF_TERN(O,B,A)    ((B) MINUS_TERN0(O,A))  // ((B) (OPTION ? '- (A)' : '<nul>'))
#define MUL_TERN(O,B,A)     ((B) MUL_TERN1(O,A))    // ((B) (OPTION ? '* (A)' : '<nul>'))
#define DIV_TERN(O,B,A)     ((B) DIV_TERN1(O,A))    // ((B) (OPTION ? '/ (A)' : '<nul>'))

// Macros to support pins/buttons exist testing
#define PIN_EXISTS(PN)      (defined(PN##_PIN) && PN##_PIN >= 0)
#define _PINEX_1            PIN_EXISTS
#define PINS_EXIST(V...)    DO(PINEX,&&,V)
#define ANY_PIN(V...)       DO(PINEX,||,V)

#define BUTTON_EXISTS(BN)   (defined(BTN_##BN) && BTN_##BN >= 0)
#define _BTNEX_1            BUTTON_EXISTS
#define BUTTONS_EXIST(V...) DO(BTNEX,&&,V)
#define ANY_BUTTON(V...)    DO(BTNEX,||,V)

// Value helper macros
#define WITHIN(N,L,H)       ((N) >= (L) && (N) <= (H))
#define ISEOL(C)            ((C) == '\n' || (C) == '\r')
#define NUMERIC(a)          WITHIN(a, '0', '9')
#define DECIMAL(a)          (NUMERIC(a) || a == '.')
#define HEXCHR(a)           (NUMERIC(a) ? (a) - '0' : WITHIN(a, 'a', 'f') ? ((a) - 'a' + 10)  : WITHIN(a, 'A', 'F') ? ((a) - 'A' + 10) : -1)
#define NUMERIC_SIGNED(a)   (NUMERIC(a) || (a) == '-' || (a) == '+')
#define DECIMAL_SIGNED(a)   (DECIMAL(a) || (a) == '-' || (a) == '+')

// Array shorthand
#define COUNT(a)            (sizeof(a)/sizeof(*a))
#define ZERO(a)             memset((void*)a,0,sizeof(a))
#define COPY(a,b) do{ \
    static_assert(sizeof(a[0]) == sizeof(b[0]), "COPY: '" STRINGIFY(a) "' and '" STRINGIFY(b) "' types (sizes) don't match!"); \
    memcpy(&a[0],&b[0],_MIN(sizeof(a),sizeof(b))); \
  }while(0)

// Expansion of some code
#define CODE_16( A,B,C,D,E,F,G,H,I,J,K,L,M,N,O,P,...) A; B; C; D; E; F; G; H; I; J; K; L; M; N; O; P
#define CODE_15( A,B,C,D,E,F,G,H,I,J,K,L,M,N,O,...) A; B; C; D; E; F; G; H; I; J; K; L; M; N; O
#define CODE_14( A,B,C,D,E,F,G,H,I,J,K,L,M,N,...) A; B; C; D; E; F; G; H; I; J; K; L; M; N
#define CODE_13( A,B,C,D,E,F,G,H,I,J,K,L,M,...) A; B; C; D; E; F; G; H; I; J; K; L; M
#define CODE_12( A,B,C,D,E,F,G,H,I,J,K,L,...) A; B; C; D; E; F; G; H; I; J; K; L
#define CODE_11( A,B,C,D,E,F,G,H,I,J,K,...) A; B; C; D; E; F; G; H; I; J; K
#define CODE_10( A,B,C,D,E,F,G,H,I,J,...) A; B; C; D; E; F; G; H; I; J
#define CODE_9( A,B,C,D,E,F,G,H,I,...) A; B; C; D; E; F; G; H; I
#define CODE_8( A,B,C,D,E,F,G,H,...) A; B; C; D; E; F; G; H
#define CODE_7( A,B,C,D,E,F,G,...) A; B; C; D; E; F; G
#define CODE_6( A,B,C,D,E,F,...) A; B; C; D; E; F
#define CODE_5( A,B,C,D,E,...) A; B; C; D; E
#define CODE_4( A,B,C,D,...) A; B; C; D
#define CODE_3( A,B,C,...) A; B; C
#define CODE_2( A,B,...) A; B
#define CODE_1( A,...) A
#define CODE_0(...)
#define _CODE_N(N,V...) CODE_##N(V)
#define CODE_N(N,V...) _CODE_N(N,V)

// Expansion of some non-delimited content
#define GANG_16(A,B,C,D,E,F,G,H,I,J,K,L,M,N,O,P,...) A B C D E F G H I J K L M N O P
#define GANG_15(A,B,C,D,E,F,G,H,I,J,K,L,M,N,O,...) A B C D E F G H I J K L M N O
#define GANG_14(A,B,C,D,E,F,G,H,I,J,K,L,M,N,...) A B C D E F G H I J K L M N
#define GANG_13(A,B,C,D,E,F,G,H,I,J,K,L,M...) A B C D E F G H I J K L M
#define GANG_12(A,B,C,D,E,F,G,H,I,J,K,L...) A B C D E F G H I J K L
#define GANG_11(A,B,C,D,E,F,G,H,I,J,K,...) A B C D E F G H I J K
#define GANG_10(A,B,C,D,E,F,G,H,I,J,...) A B C D E F G H I J
#define GANG_9( A,B,C,D,E,F,G,H,I,...) A B C D E F G H I
#define GANG_8( A,B,C,D,E,F,G,H,...) A B C D E F G H
#define GANG_7( A,B,C,D,E,F,G,...) A B C D E F G
#define GANG_6( A,B,C,D,E,F,...) A B C D E F
#define GANG_5( A,B,C,D,E,...) A B C D E
#define GANG_4( A,B,C,D,...) A B C D
#define GANG_3( A,B,C,...) A B C
#define GANG_2( A,B,...) A B
#define GANG_1( A,...) A
#define GANG_0(...)
#define _GANG_N(N,V...) GANG_##N(V)
#define GANG_N(N,V...) _GANG_N(N,V)
#define GANG_N_1(N,K) _GANG_N(N,K,K,K,K,K,K,K,K,K,K,K,K,K,K,K,K)

// Expansion of some list items
#define LIST_26(A,B,C,D,E,F,G,H,I,J,K,L,M,N,O,P,Q,R,S,T,U,V,W,X,Y,Z,...) A,B,C,D,E,F,G,H,I,J,K,L,M,N,O,P,Q,R,S,T,U,V,W,X,Y,Z
#define LIST_25(A,B,C,D,E,F,G,H,I,J,K,L,M,N,O,P,Q,R,S,T,U,V,W,X,Y,...) A,B,C,D,E,F,G,H,I,J,K,L,M,N,O,P,Q,R,S,T,U,V,W,X,Y
#define LIST_24(A,B,C,D,E,F,G,H,I,J,K,L,M,N,O,P,Q,R,S,T,U,V,W,X,...) A,B,C,D,E,F,G,H,I,J,K,L,M,N,O,P,Q,R,S,T,U,V,W,X
#define LIST_23(A,B,C,D,E,F,G,H,I,J,K,L,M,N,O,P,Q,R,S,T,U,V,W,...) A,B,C,D,E,F,G,H,I,J,K,L,M,N,O,P,Q,R,S,T,U,V,W
#define LIST_22(A,B,C,D,E,F,G,H,I,J,K,L,M,N,O,P,Q,R,S,T,U,V,...) A,B,C,D,E,F,G,H,I,J,K,L,M,N,O,P,Q,R,S,T,U,V
#define LIST_21(A,B,C,D,E,F,G,H,I,J,K,L,M,N,O,P,Q,R,S,T,U,...) A,B,C,D,E,F,G,H,I,J,K,L,M,N,O,P,Q,R,S,T,U
#define LIST_20(A,B,C,D,E,F,G,H,I,J,K,L,M,N,O,P,Q,R,S,T,...) A,B,C,D,E,F,G,H,I,J,K,L,M,N,O,P,Q,R,S,T
#define LIST_19(A,B,C,D,E,F,G,H,I,J,K,L,M,N,O,P,Q,R,S,...) A,B,C,D,E,F,G,H,I,J,K,L,M,N,O,P,Q,R,S
#define LIST_18(A,B,C,D,E,F,G,H,I,J,K,L,M,N,O,P,Q,R,...) A,B,C,D,E,F,G,H,I,J,K,L,M,N,O,P,Q,R
#define LIST_17(A,B,C,D,E,F,G,H,I,J,K,L,M,N,O,P,Q,...) A,B,C,D,E,F,G,H,I,J,K,L,M,N,O,P,Q
#define LIST_16(A,B,C,D,E,F,G,H,I,J,K,L,M,N,O,P,...) A,B,C,D,E,F,G,H,I,J,K,L,M,N,O,P
#define LIST_15(A,B,C,D,E,F,G,H,I,J,K,L,M,N,O,...) A,B,C,D,E,F,G,H,I,J,K,L,M,N,O
#define LIST_14(A,B,C,D,E,F,G,H,I,J,K,L,M,N,...) A,B,C,D,E,F,G,H,I,J,K,L,M,N
#define LIST_13(A,B,C,D,E,F,G,H,I,J,K,L,M,...) A,B,C,D,E,F,G,H,I,J,K,L,M
#define LIST_12(A,B,C,D,E,F,G,H,I,J,K,L,...) A,B,C,D,E,F,G,H,I,J,K,L
#define LIST_11(A,B,C,D,E,F,G,H,I,J,K,...) A,B,C,D,E,F,G,H,I,J,K
#define LIST_10(A,B,C,D,E,F,G,H,I,J,...) A,B,C,D,E,F,G,H,I,J
#define LIST_9( A,B,C,D,E,F,G,H,I,...) A,B,C,D,E,F,G,H,I
#define LIST_8( A,B,C,D,E,F,G,H,...) A,B,C,D,E,F,G,H
#define LIST_7( A,B,C,D,E,F,G,...) A,B,C,D,E,F,G
#define LIST_6( A,B,C,D,E,F,...) A,B,C,D,E,F
#define LIST_5( A,B,C,D,E,...) A,B,C,D,E
#define LIST_4( A,B,C,D,...) A,B,C,D
#define LIST_3( A,B,C,...) A,B,C
#define LIST_2( A,B,...) A,B
#define LIST_1( A,...) A
#define LIST_0(...)

#define _LIST_N(N,V...) LIST_##N(V)
#define LIST_N(N,V...) _LIST_N(N,V)
#define LIST_N_1(N,K) _LIST_N(N,K,K,K,K,K,K,K,K,K,K,K,K,K,K,K,K,K,K,K,K)
#define ARRAY_N(N,V...) { _LIST_N(N,V) }
#define ARRAY_N_1(N,K)  { LIST_N_1(N,K) }

#define _JOIN_1(O)         (O)
#define JOIN_N(N,C,V...)   (DO(JOIN,C,LIST_N(N,V)))

#define NOOP (void(0))

#define CEILING(x,y) (((x) + (y) - 1) / (y))

#undef ABS
#ifdef __cplusplus
  template <class T> static constexpr const T ABS(const T v) { return v >= 0 ? v : -v; }
#else
  #define ABS(a) ({__typeof__(a) _a = (a); _a >= 0 ? _a : -_a;})
#endif

#define UNEAR_ZERO(x) ((x) < 0.000001f)
#define NEAR_ZERO(x) WITHIN(x, -0.000001f, 0.000001f)
#define NEAR(x,y) NEAR_ZERO((x)-(y))

#define RECIPROCAL(x) (NEAR_ZERO(x) ? 0 : (1 / float(x)))
#define FIXFLOAT(f)  ({__typeof__(f) _f = (f); _f + (_f < 0 ? -0.0000005f : 0.0000005f);})

//
// Maths macros that can be overridden by HAL
//
#define ACOS(x)     acosf(x)
#define ATAN2(y, x) atan2f(y, x)
#define POW(x, y)   powf(x, y)
#define SQRT(x)     sqrtf(x)
#define RSQRT(x)    (1.0f / sqrtf(x))
#define CEIL(x)     ceilf(x)
#define FLOOR(x)    floorf(x)
#define TRUNC(x)    truncf(x)
#define LROUND(x)   lroundf(x)
#define FMOD(x, y)  fmodf(x, y)
#define HYPOT(x,y)  SQRT(HYPOT2(x,y))

// Use NUM_ARGS(__VA_ARGS__) to get the number of variadic arguments
#define _NUM_ARGS(_,n,m,l,k,j,i,h,g,f,e,d,c,b,a,Z,Y,X,W,V,U,T,S,R,Q,P,O,N,M,L,K,J,I,H,G,F,E,D,C,B,A,OUT,...) OUT
#define NUM_ARGS(V...) _NUM_ARGS(0,V,40,39,38,37,36,35,34,33,32,31,30,29,28,27,26,25,24,23,22,21,20,19,18,17,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0)

// Use TWO_ARGS(__VA_ARGS__) to get whether there are 1, 2, or >2 arguments
#define _TWO_ARGS(_,n,m,l,k,j,i,h,g,f,e,d,c,b,a,Z,Y,X,W,V,U,T,S,R,Q,P,O,N,M,L,K,J,I,H,G,F,E,D,C,B,A,OUT,...) OUT
#define TWO_ARGS(V...) _TWO_ARGS(0,V,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,2,1,0)


#define BCD2DEC(bcd) ((bcd & 0x0F) + ((bcd >> 4) * 10)) // Convert BCD to decimal

#define ERROR_REQUIRED_DEFINE(x) static_assert(1, #x "is not defined. Please define it in your hardware configuration." #x) // Error message for required define

