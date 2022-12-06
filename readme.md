# Filesystem

This is an implementation for a software disk system.

## To comple and run
```
gcc -g -o filesystem.out testfile.c filesystem.c softwaredisk.c
```

## Layouts:

Inode Layout
   - 2 bytes Mode 
   - 2 bytes file size in blocks 
   - 13 2 byte direct pointers 
   - 1 2 byte direct pointer 

Directory Entry Layout:
 
-    2 bytes block number/pointer to inode 0 go until 511
-    2 bytes size of file name
-  507 bytes for file name
-    1 byte null terminator

## functions

`set_bitmap(unsigned long bitmap_type, int index)`

- Marks the bit at index in the bitmap_type as free

`unset_bitmap(unsigned long bitmap_type, int index)`

- Marks the bit at index in the bitmap_type as used

`get_free_index(unsigned long bitmap_type)`

- Returns the first free index of Inode or Data Block indicated by bitmap

`update_directory_entry_block(unsigned long index, Directory_Entry_Block *buff)`

- updates/writes the directory_entry_block at index `index`

`update_inode_block(unsigned long index, Inode_Block *buff)`

- updates/writes the inode_block at index `index`.

`get_directory_entry_block(unsigned long index, Directory_Entry_Block *buff)`

- eturns an Directory_Entry_Block given an index

`get_inode_block(int index, Inode_Block *buff)`

- Returns an Inode_Block given an index

`hard_delete(uint16_t index)`

- sets all the data in a datablock block to 0s

`check_structure_alignment()`

- Checks if the filesystem has been initialized correctly.

`fs_print_error(void)`

- prints any `FSERROR`

`open_file(char *name, FileMode mode)`

- facilitates opening a file

`create_file(char *name)`

- facilitates createion of a new file

`read_file(File file, void *buf, unsigned long numbytes)`

- facilitates reading of a file

`write_file(File file, void *buf, unsigned long numbytes)`

- facilitates writing to files

`close_file(File file)`

- closes a file

`file_exists(char *name)`

- checks to see if a particular file is stored on the disk

`file_length(File file)`

- returns the size of a file

`delete_file(char *name)`

- finds and delets a file using `hard_delete()`

`seek_file(File file, unsigned long bytepos)`

- sets the file's current possission

# Limits

 - Filenames:

   Files are limited to 507 characters (not including the null terminal)

 - Files:  
   A max of 512 files