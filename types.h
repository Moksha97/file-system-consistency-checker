#ifndef _TYPES_H_
#define _TYPES_H_

// Type definitions

typedef unsigned int   uint;
typedef unsigned short ushort;
typedef unsigned char  uchar;
typedef uint pde_t;

// File types
#define T_DIR  1   // Directory
#define T_FILE 2   // File
#define T_DEV  3   // Device

#ifndef NULL
#define NULL (0)
#endif

#endif //_TYPES_H_
