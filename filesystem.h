//
// Simple filesystem API for LSU 4103 filesystem assignment.
// (@nolaforensix), 11/2017.  Minor updates 11/2019.  Updated 6/2022.
// Modified by AAG, 11/2022

#include <stdint.h>
#include "softwaredisk.h"

#if !defined(__FILESYSTEM_4103_H__)
#define __FILESYSTEM_4103_H__

// private
// struct FileInternals;

#define MAX_FILES 512
#define DATA_BITMAP_BLOCK 0  // max SOFTWARE_DISK_BLOCK_SIZE*8 bits
#define INODE_BITMAP_BLOCK 1 // max SOFTWARE_DISK_BLOCK_SIZE*8 bits
#define FIRST_INODE_BLOCK 2
#define LAST_INODE_BLOCK 5 // 128 inodes per block,
                           // max of 4*128 = 512 inodes, thus 512 files

#define INODES_PER_BLOCK 128
#define FIRST_DIR_ENTRY_BLOCK 6
#define LAST_DIR_ENTRY_BLOCK 69
#define DIR_ENTRIES_PER_BLOCK 8 // 8 DE per block
                                // max 0f 64*8 = 512 Directory entries corresponding to max file for single-level directory structure
#define FIRST_DATA_BLOCK 70
#define LAST_DATA_BLOCK 4095
#define MAX_FILENAME_SIZE 507
#define NUM_DIRECT_INODE_BLOCKS 13
#define NUM_SINGLE_INDIRECT_BLOCKS (SOFTWARE_DISK_BLOCK_SIZE / sizeof(uint16_t))

#define MAX_FILE_SIZE (NUM_DIRECT_INODE_BLOCKS + NUM_SINGLE_INDIRECT_BLOCKS) * SOFTWARE_DISK_BLOCK_SIZE


// Custome define constructs
#define DIR_ENTRY_BLOCK_SIZE (SOFTWARE_DISK_BLOCK_SIZE/DIR_ENTRIES_PER_BLOCK)
#define INODE_BLOCK_SIZE (SOFTWARE_DISK_BLOCK_SIZE/INODES_PER_BLOCK)


typedef struct
{
    uint16_t mode;      // 2 bytes
    uint16_t file_size; // 2 bytes
    uint16_t indirect_pointers; // 2 bytes
    uint16_t direct_pointers[NUM_DIRECT_INODE_BLOCKS];
    // uint16_t *direct_pointers;  // 26 bytes: (13 * 2 bytes) (actual 2 bytes )
} Inode_Block;

typedef struct
{
    uint16_t cheap_pointers[NUM_SINGLE_INDIRECT_BLOCKS];
}Indirect_Block;

typedef struct
{
    uint16_t inode_pointer;  // 2 bytes   Note: this is just inode number
    uint16_t file_name_size; // 2 bytes
    uint8_t file_name[MAX_FILENAME_SIZE + 1];
    // uint8_t file_name[]; // 508 bytes (actual 1 byte)
} Directory_Entry_Block;

// This is just an enum to help us in our functions to know
// which block number to use when dealing with one of our two bitmaps.
//
// These two are probably not necassary but I was not sure if we needed
// then when I started working on this project initially.
// typedef enum 
// {
//     DATA_BITMAP,
//     INODE_BITMAP
// } BITMAP;

// // Redefined type to stay consistent with the originally defiend types.
// // If needed we can get rid ot this line and replace Inode_Block on
// // line 45 above with FileInternals.
// typedef Inode_Block FileInternals;

typedef struct
{
    unsigned long offset;
    Inode_Block *inode;
} FileInternals;

// file type used by user code
// typedef struct FileInternals *File;
typedef FileInternals *File;

// access mode for open_file()
typedef enum
{
    NONE,
    READ_ONLY,
    READ_WRITE
} FileMode;

// error codes set in global 'fserror' by filesystem functions
typedef enum
{
    FS_NONE,
    FS_OUT_OF_SPACE,          // the operation caused the software disk to fill up
    FS_FILE_NOT_OPEN,         // attempted read/write/close/etc. on file that isn't open
    FS_FILE_OPEN,             // file is already open. Concurrent opens are not
                              // supported and neither is deleting a file that is open.
    FS_FILE_NOT_FOUND,        // attempted open or delete of file that doesn’t exist
    FS_FILE_READ_ONLY,        // attempted write to file opened for READ_ONLY
    FS_FILE_ALREADY_EXISTS,   // attempted creation of file with existing name
    FS_EXCEEDS_MAX_FILE_SIZE, // seek or write would exceed max file size
    FS_ILLEGAL_FILENAME,      // filename begins with a null character
    FS_IO_ERROR               // something really bad happened
} FSError;

// function prototypes for filesystem API

// open existing file with pathname 'name' and access mode 'mode'.
// Current file position is set to byte 0.  Returns NULL on
// error. Always sets 'fserror' global.
File open_file(char *name, FileMode mode);

// create and open new file with pathname 'name' and (implied) access
// mode READ_WRITE.  Current file position is set to byte 0.  Returns
// NULL on error. Always sets 'fserror' global.
File create_file(char *name);

// close 'file'.  Always sets 'fserror' global.
void close_file(File file);

// read at most 'numbytes' of data from 'file' into 'buf', starting at the
// current file position.  Returns the number of bytes read. If end of file is reached,
// then a return value less than 'numbytes' signals this condition. Always sets
// 'fserror' global.
unsigned long read_file(File file, void *buf, unsigned long numbytes);

// write 'numbytes' of data from 'buf' into 'file' at the current file
// position.  Returns the number of bytes written. On an out of space
// error, the return value may be less than 'numbytes'.  Always sets
// 'fserror' global.
unsigned long write_file(File file, void *buf, unsigned long numbytes);

// sets current position in file to 'bytepos', always relative to the
// beginning of file.  Seeks past the current end of file should
// extend the file. Returns 1 on success and 0 on failure.  Always
// sets 'fserror' global.
int seek_file(File file, unsigned long bytepos);

// returns the current length of the file in bytes. Always sets
// 'fserror' global.
unsigned long file_length(File file);

// deletes the file named 'name', if it exists. Returns 1 on success,
// 0 on failure.  Always sets 'fserror' global.
int delete_file(char *name);

// determines if a file with 'name' exists and returns 1 if it exists, otherwise 0.
// Always sets 'fserror' global.
int file_exists(char *name);

// describe current filesystem error code by printing a descriptive
// message to standard error.
void fs_print_error(void);

// extra function to make sure structure alignment, data structure
// sizes, etc. on target platform are correct.  Should return 1 on
// success, 0 on failure.  This should be used in disk initialization
// to ensure that everything will work correctly.
int check_structure_alignment(void);

// filesystem error code set (set by each filesystem function)
extern FSError fserror;

#endif
