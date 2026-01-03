// Enforce pointer-free serialization in PharmaSea.
//
// Save-game + network serialization are intended to be pointer-free:
// - no Bitsery pointer linking context
// - no Bitsery smart pointer extension
//
// This header is included by `src/bitsery_include.h` to provide a cheap
// compile-time "tripwire": if anyone includes Bitsery's pointer/smart_ptr
// extension headers anywhere in the same translation unit, compilation fails.
#pragma once

// Bitsery extension header include guards:
// - vendor/bitsery/ext/pointer.h
// - vendor/bitsery/ext/std_smart_ptr.h
//
// Strategy:
// - If these headers were already included before this policy header, fail fast
//   (we cannot "un-include" them).
// - Otherwise, pre-define their include guards so later accidental includes are
//   skipped by the preprocessor. This prevents reintroducing pointer-based
//   serialization helpers in the same translation unit.

#if defined(BITSERY_EXT_POINTER_H)
#error "Pointer serialization is forbidden: <bitsery/ext/pointer.h> was included."
#else
#define BITSERY_EXT_POINTER_H 1
#endif

#if defined(BITSERY_EXT_STD_SMART_PTR_H)
#error "Smart pointer serialization is forbidden: <bitsery/ext/std_smart_ptr.h> was included."
#else
#define BITSERY_EXT_STD_SMART_PTR_H 1
#endif

