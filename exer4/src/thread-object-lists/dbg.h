/**
 * Set a debugging macros adapted  from Zed Shaw's book, Learn C the Hard Way.
 *
 * debug level messages be stopped by adding #define NDEBUG or by compiling with -DNDEBUG
 *
 */
#ifndef __dbg_h__
#define __dbg_h__

#include <stdio.h>
#include <errno.h>
#include <string.h>

#ifdef NDEBUG
#define debug(M, ...)
#else
#define debug(M, ...) printf("DEBUG %s:%d: " M "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#endif

#define clean_errno() (errno == 0 ? "None" : strerror(errno))
#define log_err(M, ...) printf("[ERROR] (%s:%d: errno: %s) " M "\n", __FILE__, __LINE__, clean_errno(), ##__VA_ARGS__)
#define log_warn(M, ...) printf( "[WARN] (%s:%d: errno: %s) " M "\n", __FILE__, __LINE__, clean_errno(), ##__VA_ARGS__)
#define log_info(M, ...) printf( "[INFO] (%s:%d) " M "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#define check(A, M, ...) if(!(A)) { log_err(M, ##__VA_ARGS__); errno=0; goto error; }
#define sentinel(M, ...)  { log_err(M, ##__VA_ARGS__); errno=0; goto error; }
#define check_mem(A) check((A), "Out of memory.")
#define check_debug(A, M, ...) if(!(A)) { debug(M, ##__VA_ARGS__); errno=0; goto error; }

#endif
