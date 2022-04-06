// Disclaimer: IMPORTANT: This Apple software is supplied to you, by Apple Inc. ("Apple"), in your
// capacity as a current, and in good standing, Licensee in the MFi Licensing Program. Use of this
// Apple software is governed by and subject to the terms and conditions of your MFi License,
// including, but not limited to, the restrictions specified in the provision entitled "Public
// Software", and is further subject to your agreement to the following additional terms, and your
// agreement that the use, installation, modification or redistribution of this Apple software
// constitutes acceptance of these additional terms. If you do not agree with these additional terms,
// you may not use, install, modify or redistribute this Apple software.
//
// Subject to all of these terms and in consideration of your agreement to abide by them, Apple grants
// you, for as long as you are a current and in good-standing MFi Licensee, a personal, non-exclusive
// license, under Apple's copyrights in this Apple software (the "Apple Software"), to use,
// reproduce, and modify the Apple Software in source form, and to use, reproduce, modify, and
// redistribute the Apple Software, with or without modifications, in binary form, in each of the
// foregoing cases to the extent necessary to develop and/or manufacture "Proposed Products" and
// "Licensed Products" in accordance with the terms of your MFi License. While you may not
// redistribute the Apple Software in source form, should you redistribute the Apple Software in binary
// form, you must retain this notice and the following text and disclaimers in all such redistributions
// of the Apple Software. Neither the name, trademarks, service marks, or logos of Apple Inc. may be
// used to endorse or promote products derived from the Apple Software without specific prior written
// permission from Apple. Except as expressly stated in this notice, no other rights or licenses,
// express or implied, are granted by Apple herein, including but not limited to any patent rights that
// may be infringed by your derivative works or by other works in which the Apple Software may be
// incorporated. Apple may terminate this license to the Apple Software by removing it from the list
// of Licensed Technology in the MFi License, or otherwise in accordance with the terms of such MFi License.
//
// Unless you explicitly state otherwise, if you provide any ideas, suggestions, recommendations, bug
// fixes or enhancements to Apple in connection with this software ("Feedback"), you hereby grant to
// Apple a non-exclusive, fully paid-up, perpetual, irrevocable, worldwide license to make, use,
// reproduce, incorporate, modify, display, perform, sell, make or have made derivative works of,
// distribute (directly or indirectly) and sublicense, such Feedback in connection with Apple products
// and services. Providing this Feedback is voluntary, but if you do provide Feedback to Apple, you
// acknowledge and agree that Apple may exercise the license granted above without the payment of
// royalties or further consideration to Participant.

// The Apple Software is provided by Apple on an "AS IS" basis. APPLE MAKES NO WARRANTIES, EXPRESS OR
// IMPLIED, INCLUDING WITHOUT LIMITATION THE IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY
// AND FITNESS FOR A PARTICULAR PURPOSE, REGARDING THE APPLE SOFTWARE OR ITS USE AND OPERATION ALONE OR
// IN COMBINATION WITH YOUR PRODUCTS.
//
// IN NO EVENT SHALL APPLE BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) ARISING IN ANY WAY OUT OF THE USE, REPRODUCTION, MODIFICATION
// AND/OR DISTRIBUTION OF THE APPLE SOFTWARE, HOWEVER CAUSED AND WHETHER UNDER THEORY OF CONTRACT, TORT
// (INCLUDING NEGLIGENCE), STRICT LIABILITY OR OTHERWISE, EVEN IF APPLE HAS BEEN ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// Copyright (C) 2015-2021 Apple Inc. All Rights Reserved.

#ifndef HAP_BASE_COMPILER_ABSTRACTION_H
#define HAP_BASE_COMPILER_ABSTRACTION_H

#ifdef __cplusplus
extern "C" {
#endif

// Fallback for compilers that do not support querying for attributes.
#ifndef __has_attribute
#define __has_attribute(x) 0
#endif

// Fallback for compilers that do not support C++20 check for attributes.
#ifndef __has_cpp_attribute
#define __has_cpp_attribute(x) 0
#endif

// Fallback for compilers that do not support querying for builtins.
#ifndef __has_builtin
#define __has_builtin(x) 0
#endif

// Fallback for compilers that do not support querying for extensions.
#ifndef __has_extension
#define __has_extension(x) 0
#endif

// Fallback for compilers that do not support querying for features.
#ifndef __has_feature
#define __has_feature(x) 0
#endif

#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

// Fallback for compilers that do not support the nullability feature.
#if !__has_feature(nullability)
#define _Nonnull
#define _Nullable
#endif

// Fallback for compilers that do not support the inline keyword.
#if defined(_MSC_VER)
#define inline __inline
#endif

// Fallback for compilers that do not support the __func__ definition.
#if defined(_MSC_VER)
#define __func__ __FUNCTION__
#endif

// Fallback if HAP_FILE is not defined. __FILE__ may include absolute paths.
#ifndef HAP_FILE
#define HAP_FILE __FILE__
#endif

//----------------------------------------------------------------------------------------------------------------------

/**
 * Specifies the minimum alignment of a variable or field.
 *
 * @param      numBytes             Minimum alignment in bytes.
 */
/**@{*/
#if defined(_MSC_VER)
#define HAP_ALIGNAS(numBytes) __declspec(align(numBytes))
#elif __has_attribute(aligned) || (defined(__GNUC__) && (__GNUC__ >= 5 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 2))) || \
        defined(__CC_ARM) || defined(__ICCARM__)

#define HAP_ALIGNAS(numBytes) __attribute__((aligned(numBytes)))
#else
// This attribute is not optional.
#endif
/**@}*/

/**
 * Indicates that a function has no side effects and that the return value depends solely on its parameters.
 *
 * - The function may NOT read global variables and may NOT dereference pointer arguments.
 */
/**@{*/
#if __has_attribute(const) || (defined(__GNUC__) && (__GNUC__ >= 3 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 5))) || \
        defined(__CC_ARM) || defined(__ICCARM__)

#define HAP_CONST __attribute__((const))
#else
#define HAP_CONST
#endif
/**@}*/

/**
 * Marks code as deprecated. A warning is emitted if the code is used.
 */
/**@{*/
#if __has_attribute(deprecated) || (defined(__GNUC__) && (__GNUC__ >= 5 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 8))) || \
        defined(__CC_ARM) || defined(__ICCARM__)

#define HAP_DEPRECATED __attribute__((deprecated))
#else
#define HAP_DEPRECATED
#endif
/**@}*/

/**
 * Marks code as deprecated. A warning is emitted if the code is used.
 *
 * @param      message              Message to show as part of the warning.
 */
/**@{*/
#if __has_attribute(deprecated) || (defined(__GNUC__) && (__GNUC__ >= 5 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 8))) || \
        defined(__CC_ARM) || defined(__ICCARM__)

#define HAP_DEPRECATED_MSG(message) __attribute__((deprecated(message)))
#else
#define HAP_DEPRECATED_MSG(message)
#endif
/**@}*/

/**
 * Marks an enumerator as deprecated. A warning is emitted if it is used.
 *
 * @param      message              Message to show as part of the warning.
 */
/**@{*/
#if __has_extension(enumerator_attributes) || (defined(__GNUC__) && (__GNUC__ >= 6))

#define HAP_DEPRECATED_ENUM_MSG(message) __attribute__((deprecated(message)))
#else
#define HAP_DEPRECATED_ENUM_MSG(message)
#endif

/**
 * Emits a warning if calls to the function meet the given criteria.
 *
 * - Warning is only emitted if the criteria can be evaluated at compile-time.
 */
/**@{*/
#if __has_attribute(diagnose_if)
#define HAP_DIAGNOSE_WARNING(...) __attribute__((diagnose_if(__VA_ARGS__, "warning")))
#else
#define HAP_DIAGNOSE_WARNING(...)
#endif
/**@}*/

/**
 * Emits an error if calls to the function meet the given criteria.
 *
 * - Error is only emitted if the criteria can be evaluated at compile-time.
 */
/**@{*/
#if __has_attribute(diagnose_if)
#define HAP_DIAGNOSE_ERROR(...) __attribute__((diagnose_if(__VA_ARGS__, "error")))
#else
#define HAP_DIAGNOSE_ERROR(...)
#endif
/**@}*/

/**
 * Marks a function to take printf-like arguments. If there is a mismatch between the format string and subsequent
 * function parameter types a warning is emitted.
 *
 * - Argument numbers start at 1.
 *
 * @param      formatArg            Argument number of the format string.
 * @param      firstVarArg          Argument number where variadic arguments start (or 0 if not used).
 */
/**@{*/
#if __has_attribute(format) || (defined(__GNUC__) && (__GNUC__ >= 5 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6))) || \
        defined(__CC_ARM)

#define HAP_PRINTFLIKE(formatArg, firstVarArg) __attribute__((format(printf, formatArg, firstVarArg)))
#else
#define HAP_PRINTFLIKE(formatArg, firstVarArg)
#endif
/**@}*/

/**
 * Indicates that a pointer argument does not escape.
 *
 * - No reference that is derived from the pointer parameter value will survive after the function returns.
 */
/**@{*/
#if __has_attribute(noescape)
#define HAP_NOESCAPE __attribute__((noescape))
#else
#define HAP_NOESCAPE
#endif
/**@}*/

/**
 * Marks a function that never returns once it is called.
 */
/**@{*/
#if defined(_MSC_VER)
#define HAP_NORETURN __declspec(noreturn)
#elif __has_attribute(noreturn) || (defined(__GNUC__) && (__GNUC__ >= 5 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 2))) || \
        defined(__CC_ARM) || defined(__ICCARM__)

#define HAP_NORETURN __attribute__((noreturn))
#else
// This attribute is not optional.
#endif

/**
 * Indicates that a function has no side effects.
 *
 * - The function may read global variables and may dereference pointer arguments.
 */
/**@{*/
#if __has_attribute(pure) || (defined(__GNUC__) && __GNUC__ >= 3) || defined(__CC_ARM) || defined(__ICCARM__)

#define HAP_NO_SIDE_EFFECTS __attribute__((pure))
#else
#define HAP_NO_SIDE_EFFECTS
#endif
/**@}*/

/**
 * Marks a declaration as unused.
 */
/**@{*/
#if __has_attribute(unused) || (defined(__GNUC__) && (__GNUC__ >= 5 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 2))) || \
        defined(__CC_ARM) || defined(__ICCARM__)

#define HAP_UNUSED __attribute__((unused))
#else
#define HAP_UNUSED
#endif
/**@}*/

/**
 * Warns the callers of a function if the result is never used.
 */
/**@{*/
#if __has_attribute(warn_unused_result) || \
        (defined(__GNUC__) && (__GNUC__ >= 5 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 2))) || defined(__CC_ARM)

#define HAP_RESULT_USE_CHECK     //__attribute__((warn_unused_result))
#else
#define HAP_RESULT_USE_CHECK
#endif
/**@}*/

/**
 * Annotate a 'case' section that should fall-through into the next one. This allows -Wimplicit-fallthrough
 * to check where accidental fallthrough could occur.
 */
/**@{*/
#if defined(__cplusplus) && __has_cpp_attribute(fallthrough)
#define HAP_FALLTHROUGH [[fallthrough]]
#elif __has_attribute(fallthrough) || (defined(__GNUC__) && (__GNUC__ >= 7))
#define HAP_FALLTHROUGH __attribute__((fallthrough))
#else
#define HAP_FALLTHROUGH ((void) 0)
#endif
/**@}*/

//----------------------------------------------------------------------------------------------------------------------

/**
 * Poisons an identifier so that it can no longer be used.
 *
 * @param      identifier           Identifier to poison.
 */
/**@{*/
#if !defined(HAP_DISALLOW_USE_IGNORED) || !HAP_DISALLOW_USE_IGNORED
#if defined(__clang__) || (defined(__GNUC__) && (__GNUC__ >= 5 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)))
/**@cond */
#define HAP_DISALLOW_USE_(directive) _Pragma(#directive)
/**@endcond */
#define HAP_DISALLOW_USE(identifier) HAP_DISALLOW_USE_(GCC poison identifier)
#else
#define HAP_DISALLOW_USE(identifier)
#endif
#else
#define HAP_DISALLOW_USE(identifier)
#endif
/**@}*/

//----------------------------------------------------------------------------------------------------------------------

/**
 * Returns the offset of a member within a structure.
 *
 * @param      type                 Standard-layout data structure or union type.
 * @param      member               Member within the type of which the offset is retrieved.
 *
 * @return Offset of the given member in bytes from the beginning of an object of the specified type.
 */
/**@{*/
#if __has_builtin(__builtin_offsetof) || defined(__GNUC__)

#define HAP_OFFSETOF(type, member) __builtin_offsetof(type, member)
#else
#define HAP_OFFSETOF(type, member) offsetof(type, member)
#endif
/**@}*/

//----------------------------------------------------------------------------------------------------------------------

/**
 * @defgroup CompilerDiagnostics Managing compiler diagnostics.
 *
 * These macros may be used to control compiler diagnostics locally.
 *
 * **Example**

   @code{.c}

   HAP_DIAGNOSTIC_PUSH
   HAP_DIAGNOSTIC_IGNORED_CLANG("-Wfoo")
   // Code that would trigger -Wfoo when compiling with Clang.
   HAP_DIAGNOSTIC_POP

   @endcode
 */
/**@{*/

/**
 * Saves the current compiler diagnostic state.
 */
/**@{*/
#if defined(_MSC_VER)
#define HAP_DIAGNOSTIC_PUSH __pragma(warning(push))
#elif defined(__CC_ARM)
#define HAP_DIAGNOSTIC_PUSH _Pragma("push")
#elif defined(__clang__) || (defined(__GNUC__) && (__GNUC__ >= 5 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 8)))
#define HAP_DIAGNOSTIC_PUSH _Pragma("GCC diagnostic push")
#else
#define HAP_DIAGNOSTIC_PUSH
#endif
/**@}*/

/**
 * Restores the saved compiler diagnostic state.
 */
/**@{*/
#if defined(_MSC_VER)
#define HAP_DIAGNOSTIC_POP __pragma(warning(pop))
#elif defined(__CC_ARM)
#define HAP_DIAGNOSTIC_POP _Pragma("pop")
#elif defined(__clang__) || (defined(__GNUC__) && (__GNUC__ >= 5 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 8)))
#define HAP_DIAGNOSTIC_POP _Pragma("GCC diagnostic pop")
#else
#define HAP_DIAGNOSTIC_POP
#endif
/**@}*/

/**
 * Ignores a Clang compiler warning.
 *
 * @param      warning              Warning to ignore. Must be passed as string, e.g., "-Wfoo".
 */
/**@{*/
#if defined(__clang__)
/**@cond */
#define HAP_DIAGNOSTIC_IGNORED_CLANG_(directive) _Pragma(#directive)
/**@endcond */
#define HAP_DIAGNOSTIC_IGNORED_CLANG(warning) HAP_DIAGNOSTIC_IGNORED_CLANG_(GCC diagnostic ignored warning)
#else
#define HAP_DIAGNOSTIC_IGNORED_CLANG(warning)
#endif
/**@}*/

/**
 * Ignores a GCC compiler warning.
 *
 * @param      warning              Warning to ignore. Must be passed as string, e.g., "-Wfoo".
 */
/**@{*/
#if (defined(__GNUC__) && (__GNUC__ >= 5 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 8))) && !defined(__clang)
/**@cond */
#define HAP_DIAGNOSTIC_IGNORED_GCC_(directive) _Pragma(#directive)
/**@endcond */
#define HAP_DIAGNOSTIC_IGNORED_GCC(warning) HAP_DIAGNOSTIC_IGNORED_GCC_(GCC diagnostic ignored warning)
#else
#define HAP_DIAGNOSTIC_IGNORED_GCC(warning)
#endif
/**@}*/

/**
 * Ignores an ArmCC compiler warning.
 *
 * @param      warning              Warning to ignore. Numeric warning code must be passed, e.g., 42.
 */
/**@{*/
#if defined(__CC_ARM)
/**@cond */
#define HAP_DIAGNOSTIC_IGNORED_ARMCC_(directive) _Pragma(#directive)
/**@endcond */
#define HAP_DIAGNOSTIC_IGNORED_ARMCC(warning) HAP_DIAGNOSTIC_IGNORED_ARMCC_(diag_suppress warning)
#else
#define HAP_DIAGNOSTIC_IGNORED_ARMCC(warning)
#endif
/**@}*/

/**
 * Ignores an IAR compiler warning.
 *
 * @param      warning              Warning to ignore. Warning code must be passed, e.g., Pm42.
 */
/**@{*/
#if defined(__ICCARM__)
/**@cond */
#define HAP_DIAGNOSTIC_IGNORED_ICCARM_(directive) _Pragma(#directive)
/**@endcond */
#define HAP_DIAGNOSTIC_IGNORED_ICCARM(warning) HAP_DIAGNOSTIC_IGNORED_ICCARM_(diag_suppress = warning)
#else
#define HAP_DIAGNOSTIC_IGNORED_ICCARM(warning)
#endif
/**@}*/

/**
 * Restores default settings for an IAR compiler warning.
 *
 * @param      warning              Warning to restore to default settings for. Warning code must be passed, e.g., Pm42.
 */
/**@{*/
#if defined(__ICCARM__)
/**@cond */
#define HAP_DIAGNOSTIC_RESTORE_ICCARM_(directive) _Pragma(#directive)
/**@endcond */
#define HAP_DIAGNOSTIC_RESTORE_ICCARM(warning) HAP_DIAGNOSTIC_IGNORED_ICCARM_(diag_default = warning)
#else
#define HAP_DIAGNOSTIC_RESTORE_ICCARM(warning)
#endif
/**@}*/

/**
 * Ignores an MSVC compiler warning.
 *
 * @param      warning_             Warning to ignore. Numeric warning code must be passed, e.g., 42.
 */
/**@{*/
#if defined(_MSC_VER)
#define HAP_DIAGNOSTIC_IGNORED_MSVC(warning_) __pragma(warning(disable : warning_))
#else
#define HAP_DIAGNOSTIC_IGNORED_MSVC(warning_)
#endif
/**@}*/

/**@}*/

//----------------------------------------------------------------------------------------------------------------------

/**
 * Static assertion.
 *
 * @param      condition            Condition to statically check. If it fails, an error will be thrown when compiling.
 * @param      identifier           Identifier to display as part of the compilation error.
 */
#if __has_feature(cxx_static_assert)
#define HAP_STATIC_ASSERT(condition, identifier) static_assert((condition), #identifier ": " #condition)
#elif __has_feature(c_static_assert)
#define HAP_STATIC_ASSERT(condition, identifier) _Static_assert((condition), #identifier ": " #condition)
#else
#define HAP_STATIC_ASSERT(condition, identifier) typedef char HAPStaticAssert_##identifier[(condition) ? 1 : -1]
#endif

//----------------------------------------------------------------------------------------------------------------------

/**
 * @defgroup PackedEnum Support for packed enumerations.
 *
 * - Standard C defines enum as int but some compilers provide options to change this behaviour globally.
 *   Compiling the library using different options than the application would result in ABI incompatibility.
 *   Therefore, all enumerations in public interfaces are marked as packed using the HAP_ENUM macros.
 *   This overrides the globally selected rules and ensures that short enums are used.
 *
 * **Example**

   @code{.c}

   HAP_ENUM_BEGIN(uint8_t, HAPFoo) {
       kHAPFoo_Xxx = 1,
       kHAPFoo_Yyy
   } HAP_ENUM_END(uint8_t, HAPFoo)

   @endcode
 */
/**@{*/

/**
 * Begins definition of a packed enumeration.
 *
 * @param      type                 Smallest integer type that can encode all values of the enumeration.
 * @param      name                 Name of the enumeration.
 */
/**@{*/
#if defined(_MSC_VER)
#define HAP_ENUM_BEGIN(type, name) \
    typedef type name; \
    enum
#else
#define HAP_ENUM_BEGIN(type, name) typedef enum __attribute__((packed))
#endif
/**@}*/

/**
 * Completes definition of a packed enumeration.
 *
 * @param      type                 Smallest integer type that can encode all values of the enumeration.
 * @param      name                 Name of the enumeration.
 */
/**@{*/
#if defined(_MSC_VER)
#define HAP_ENUM_END(type, name) \
    ; \
    HAP_STATIC_ASSERT(sizeof(name) == sizeof(type), name)
#else
#define HAP_ENUM_END(type, name) \
    name; \
    HAP_STATIC_ASSERT(sizeof(name) == sizeof(type), name)
#endif
/**@}*/

/**@}*/

/**
 * @defgroup OptionSets Support for option sets.
 *
 * **Example**

   @code{.c}

   HAP_OPTIONS_BEGIN(uint8_t, HAPFoo) {
       kHAPFoo_None = 0,
       kHAPFoo_Xxx = 1U << 0U,
       kHAPFoo_Yyy = 1U << 1U
   } HAP_OPTIONS_END(uint8_t, HAPFoo)

   @endcode
 */
/**@{*/

/**
 * Begins definition of a packed option set.
 *
 * @param      type                 Smallest integer type that can encode all values of the option set.
 * @param      name                 Name of the option set.
 */
/**@{*/
#if defined(_MSC_VER)
#define HAP_OPTIONS_BEGIN(type, name) \
    typedef type name; \
    enum
#else
#define HAP_OPTIONS_BEGIN(type, name) \
    typedef type name; \
    enum __attribute__((packed))
#endif
/**@}*/

/**
 * Completes definition of a packed option set.
 *
 * @param      type                 Smallest integer type that can encode all values of the option set.
 * @param      name                 Name of the option set.
 */
/**@{*/
#if defined(_MSC_VER)
#define HAP_OPTIONS_END(type, name) \
    ; \
    HAP_STATIC_ASSERT(sizeof(name) == sizeof(type), name)
#else
#define HAP_OPTIONS_END(type, name) \
    ; \
    HAP_STATIC_ASSERT(sizeof(name) == sizeof(type), name)
#endif
/**@}*/

/**@}*/

//----------------------------------------------------------------------------------------------------------------------

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#ifdef __cplusplus
}
#endif

#endif
