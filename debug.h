#ifndef __FROST_DEBUG__
#define __FROST_DEBUG__
#include <stdio.h>
enum debug_type {
    ENT = 0,
    LEV,
    MES
};
#ifdef __DEBUG__

#define __FIRST_ARG__(N, ...) N
#define __REST_ARGS__(N, ...) __VA_ARGS__
#define FIRST_ARG(args) __FIRST_ARG__ args
#define REST_ARGS(args) __REST_ARGS__ args
#define DEBUG_ENT() printf("DEBUG: Entering %s\n", __FUNCTION__);
#define DEBUG_LEA() printf("DEBUG: Leaving %s\n", __FUNCTION__);
#define DEBUG_MESG(fmt, ...)                                            \
    printf("DEBUG: At %d in function %s in file %s: ", __LINE__, __FUNCTION__, __FILE__); \
    printf(fmt, ## __VA_ARGS__);

// ne DEBUG(type, ...)                                                \
//                                                                    \
// witch (type) {                                                     \
// ase ENT:                                                           \
//    DEBUG_ENTER_FUNC;                                               \
//    break;                                                          \
// ase LEV:                                                           \
//    DEBUG_LEAVE_FUNC;                                               \
//    break;                                                          \
// ase MES:                                                           \
//    DEBUG_MESG(FIRST_ARG((__VA_ARGS__)), REST_ARGS((__VA_ARGS__))); \
// efault:                                                            \
//    printf("Illegal debug\n");                                      \
//                                                                    \
// 
#else
#define DEBUG_ENT()
#define DEBUG_LEA()
#define DEBUG_MESG(fmt, ...)
#endif

#endif
