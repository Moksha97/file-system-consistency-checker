#ifndef _ERRORS_H_
#define _ERRORS_H_

// Type definitions
#define ERROR "ERROR: "

#define BAD_INODE "bad inode"
#define BAD_DIRECT_ADDRESS_INODE "bad direct address in inode"
#define BAD_INDIRECT_ADDRESS_INODE "bad indirect address in inode"
#define ROOT_DIR_DOES_NOT_EXIST "root directory does not exist"
#define DIRECTORY_NOT_FORMATTED_PROPERLY "directory not properly formatted"
#define MISSING_BITMAP_MARK "address used by inode but marked free in bitmap"
#define MISSING_INODE_MARK "bitmap marks block in use but it is not in use"
#define MULTIPLE_DIRECT_BLOCKS_INUSE "direct address used more than once"
#define MULTIPLE_INDIRECT_BLOCKS_INUSE "indirect address used more than once"
#define DIRECTORY_MISMATCH_INODE_INUSE "inode marked use but not found in a directory"
#define DIRECTORY_MISMATCH_INODE_FREE "inode referred to in directory but marked free"
#define BAD_REFERENCE_COUNT_FILE "bad reference count for file"
#define DIRECTORY_MULTIPLE_REFERNECE_ERROR "directory appears more than once in file system"

#define END ".\n"

#endif //_ERRORS_H_