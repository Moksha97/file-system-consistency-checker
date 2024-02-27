# file-system-consistency-checker
For xv6 file system, Implemented consistency checks for data blocks, inodes, directories.

Rules:
Each inode is either unallocated or one of the valid types (T_FILE, T_DIR, T_DEV). If not, print ERROR: bad inode.
For in-use inodes, each address that is used by the inode is valid (points to a valid datablock address within the image). If the direct block is used and is invalid, print ERROR: bad direct address in inode.; if the indirect block is in use and is invalid, print ERROR: bad indirect address in inode.
Root directory exists, its inode number is 1, and the parent of the root directory is itself. If not, print ERROR: root directory does not exist.
Each directory contains . and .. entries, and the . entry points to the directory itself. If not, print ERROR: directory not properly formatted.
For in-use inodes, each address in use is also marked in use in the bitmap. If not, print ERROR: address used by inode but marked free in bitmap.
For blocks marked in-use in bitmap, the block should actually be in-use in an inode or indirect block somewhere. If not, print ERROR: bitmap marks block in use but it is not in use.
For in-use inodes, each direct address in use is only used once. If not, print ERROR: direct address used more than once.
For in-use inodes, each indirect address in use is only used once. If not, print ERROR: indirect address used more than once.
For all inodes marked in use, each must be referred to in at least one directory. If not, print ERROR: inode marked use but not found in a directory.
For each inode number that is referred to in a valid directory, it is actually marked in use. If not, print ERROR: inode referred to in directory but marked free.
Reference counts (number of links) for regular files match the number of times file is referred to in directories (i.e., hard links work correctly). If not, print ERROR: bad reference count for file.
No extra links allowed for directories (each directory only appears in one other directory). If not, print ERROR: directory appears more than once in file system.
