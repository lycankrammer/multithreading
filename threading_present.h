/**
 * @file threading_present.h
 * @brief This header provides a thin layer for thread model detection
 * between C11 threads and POSIX Threads.
 *
 * This header tries to detect an available -- among C11 threads and Pthreads --
 * threading API. It also tries to detect the presence of C11 atomic types.
 *
 * The following macros are conditionally defined:
 * @a THREADING_USE_C11THREADS if C11 threads are present
 * @a THREADING_USE_PTHREADS if POSIX Threads are present on a Unix system
 * @a THREADING_USE_WINPTHREADS if POSIX Threads for Win32 are present
 * @a THREADING_NOT_FOUND if none of the above was detected
 * @a THREADING_USE_ATOMICS if C11 _Atomic types are present
 */
#ifndef _THREADING_MODEL_
#define _THREADING_MODEL_ 1

#if !defined(__STDC_VERSION__)
#  error "Possible non-Standard compliant implementation."
#endif

#if (__STDC_VERSION__ >= 201112L)
#  if !defined(__STDC_NO_THREADS__)
#    define THREADING_USE_C11THREADS 1
#  endif
#  if !defined(__STDC_NO_ATOMICS__)
#    define THREADING_USE_ATOMICS 1
#  endif
#endif

#if defined(__unix__)
#  include <unistd.h>
#  if defined(_POSIX_VERSION)
#    define THREADING_USE_PTHREADS 1
#  endif
#elif defined(_WIN32)
#  if defined(__WINPTHREADS_VERSION)
#    define THREADING_USE_WINPTHREADS 1
#  endif
#endif

#if !defined(THREADING_USE_C11THREADS) && !defined(THREADING_USE_PTHREADS) && !defined(THREADING_USE_WINPTHREADS)
#  define THREADING_NOT_FOUND 1
#  if defined(__GNUC__)
#    warning "No thread API found."
#  elif defined(_MSC_VER)
#    pragma message("No thread API found.")
#  endif
#endif

#endif
