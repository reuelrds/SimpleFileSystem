#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

#include "softwaredisk.h"
#include "filesystem.h"



// TODO:
// 1. make bitmaps of inodes                                    Done...
// 2. make bitmaps of free blocks                               Done...
// 3. make conversion table for local to real pointers          We do not require this
// 4. make appropreate number of blocks                         Done...
// 5. format the memory before starting                         Done...


FSError FSERROR;

/**
 * @brief some whiteboarding stuff to work out sturcture of indoes and  directory blocks.
 *
 *      each block is of size 4096 bytes
 *      we have 128 inodes per block
 *
 *      => this implies that each inode is of size 32 bytes.
 *
 *      we have 13 direct pointers and 1 indirect pointer.
 *      each pointer is of size 2 bytes
 *
 *      => this implies that out of 32 bytes 28 bytes are used by block pointers.
 *
 *
 *      Question?
 *          what about the other 4 bytes?
 *
 *      We will probably need to allocate someting to store size.
 *      => this implies that we need to look into the files and file sizes next.
 *
 *
 *      Question?
 *          whats the max file size?
 *
 *      => MAX_FILE_SIZE  defines that the max file is 8441856 bytes or 8244 KB.
 *
 *    
 *
 *      Question?
 *          what's the max software disk size?
 *
 *      => (LAST_DATA_BLOCK  + 1) * 4096 bytes
 *      => 16 MB.
 *
 *
 *      Question?
 *          Whats the best datatype to store file size?
 *
 *      => unsigned short should do the trick to keep track of file size in
 *          bytes. It's 2 bytes long and ranges from 0 to 65535.
 *
 *          Note: the size is sored in terms of number of blocks. So,
 *          with our implementation this value can only range between 0 and
 *          2061.
 *
 *
 *      => this implies that of 32 bytes 28 are for pointers, 2 for file size.
 *
 *
 *
 *       Question?
 *          What's the remaining 2 bytes for?
 *
 *       =>  This probably means that the two remaining bytes are for sotring
 *          the file mode as defined in FileMode when it its opened.
 *
 *       => LAYOUT: Inode
 *           2 bytes Mode
 *           2 bytes file size in blocks
 *          13 2 byte direct pointers
 *           1 2 byte direct pointer
 *
 *
 *      Note: The bytepos is ignored from this layout because
 *            we do not require it when the file is closed.
 *            it is only used when we open a file
 *
 *            So, we set it only when a file is created. This
 *            necessiates that we need a separate structures
 *            for Inode and the File.
 *
 *
 *
 *
 *      Question?
 *          what about directory block?
 *
 *      each dir block is of size 512 bytes
 *      MAX_FILENAME_SIZE is 507 bytes
 *
 *      So, what about the last 5 bytes?
 *
 *      Question?
 *          is one byte (the 508th byte) used to store null terminator?
 *
 *      Assumming yes, this accounts for 508 bytes.
 *
 *      We have total 512 inode blocks.
 *      So, we will need a unsigned short to store the pointer (read block nunmber).
 *
 *
 *      What about the remaining two bytes?
 *
 *         Should we assume that this ths the file name size?
 *
 *      => LAYOUT: Directory Block
 *
 *            2 bytes block number/pointer to inode 0 go until 511
 *            2 bytes size of file name
 *          507 bytes for file name
 *            1 byte null terminator
 *
 */

// Helper Functions

/// @brief Marks the bit at index in the bitmap_type as free (sets it to 1).
/// @param bitmap_type (unsigned long)  The bitmap to query. Can either be DATA_BITMAP_BLOCK or INODE_BITMAP_BLOCK
/// @param index (int) the index where we need to set the bit
/// @return 1 on success otherwise 0
int set_bitmap(unsigned long bitmap_type, int index)
{

    if (bitmap_type != DATA_BITMAP_BLOCK && bitmap_type != INODE_BITMAP_BLOCK)
    {
        return 0;
    }

    if (bitmap_type == DATA_BITMAP_BLOCK && index < FIRST_DATA_BLOCK)
    {
        return 0;
    }

    uint8_t *buffer = malloc(SOFTWARE_DISK_BLOCK_SIZE);
    read_sd_block(buffer, bitmap_type); // TODO: Check for errors returned while reading

    buffer[index] = (uint8_t)1;

    write_sd_block(buffer, bitmap_type); // TODO: Check for errors returned while reading

    free(buffer);

    return 1;
}

/// @brief Marks the bit at index in the bitmap_type as used (sets it to 0).
/// @param bitmap_type (unsigned long)  The bitmap to query. Can either be DATA_BITMAP_BLOCK or INODE_BITMAP_BLOCK
/// @param index (int) the index where we need to set the bit
/// @return 1 on success otherwise 0
int unset_bitmap(unsigned long bitmap_type, int index)
{

    if (bitmap_type != DATA_BITMAP_BLOCK && bitmap_type != INODE_BITMAP_BLOCK)
    {
        return 0;
    }

    if (bitmap_type == DATA_BITMAP_BLOCK && index < FIRST_DATA_BLOCK)
    {
        return 0;
    }

    uint8_t *buffer = malloc(SOFTWARE_DISK_BLOCK_SIZE);
    read_sd_block(buffer, bitmap_type); // TODO: Check for errors returned while reading

    buffer[index] = 0;

    write_sd_block(buffer, bitmap_type); // TODO: Check for errors returned while reading

    free(buffer);

    return 1;
}

/// @brief Returns the first free index of Inode or Data Block indicated by bitmap
/// @param bitmap_type (unsigned long)  The bitmap to query. Can either be DATA_BITMAP_BLOCK or INODE_BITMAP_BLOCK
/// @return (int) the index of the first free Inode or Data Block or -1 on error
int get_free_index(unsigned long bitmap_type)
{

    if (bitmap_type != DATA_BITMAP_BLOCK && bitmap_type != INODE_BITMAP_BLOCK)
    {
        return -1;
    }

    uint8_t *buffer = malloc(SOFTWARE_DISK_BLOCK_SIZE);
    read_sd_block(buffer, bitmap_type); // TODO: Check for errors returned while reading

    for (int index = 0; index < SOFTWARE_DISK_BLOCK_SIZE; index++)
    {

        if (buffer[index] == 1)
        {
            return index;
        }
    }

    free(buffer);

    return -1;
}

/// @brief updates/writes the directory_entry_block at index `index`.
/// @param index index of the directory_entry_block. must be between 0 and 511
/// @param buff new Directory Entry
/// @return 1 on success otherwise 0 on error.
int update_directory_entry_block(unsigned long index, Directory_Entry_Block *buff)
{
    // Compute the offsets
    unsigned long dir_entry_block_number = (unsigned long)floor(
        FIRST_DIR_ENTRY_BLOCK +
        (index / DIR_ENTRIES_PER_BLOCK)); // this found the macro block number

    unsigned long dir_entry_block_offset = (unsigned long)floor(
        index - (dir_entry_block_number * DIR_ENTRIES_PER_BLOCK)); // this finds the offset of the micro block from the macro block number

    // read the blocks
    uint8_t *block_buffer = malloc(SOFTWARE_DISK_BLOCK_SIZE);
    read_sd_block(block_buffer, dir_entry_block_number); // TODO: Check if errors are returned while reading the data

    memcpy(block_buffer + (dir_entry_block_offset * DIR_ENTRY_BLOCK_SIZE), buff, sizeof(Directory_Entry_Block));

    write_sd_block(block_buffer, dir_entry_block_number); // TODO: Check if errors are returned while reading the data

    free(block_buffer);

    return 1;
}

/// @brief updates/writes the inode_block at index `index`.
/// @param index index of the inode_block. must be between 0 and 511
/// @param buff new Inode Entry
/// @return 1 on success otherwise 0 on error.
int update_inode_block(unsigned long index, Inode_Block *buff)
{
    // Compute the offsets
    unsigned long inode_block_number = (unsigned long)floor(
        FIRST_INODE_BLOCK +
        (index / INODES_PER_BLOCK)); // this found the macro block number

    unsigned long inode_block_offset = (unsigned long)floor(
        index - (inode_block_number * INODES_PER_BLOCK)); // this finds the offset of the micro block from the macro block number

    // read the blocks
    uint8_t *block_buffer = malloc(SOFTWARE_DISK_BLOCK_SIZE);
    read_sd_block(block_buffer, inode_block_number); // TODO: Check if errors are returned while reading the data

    memcpy(block_buffer + (inode_block_offset * INODE_BLOCK_SIZE), buff, sizeof(Inode_Block));

    write_sd_block(block_buffer, inode_block_number); // TODO: Check if errors are returned while reading the data

    free(block_buffer);

    return 1;
}

/// @brief Returns an Directory_Entry_Block given an index
/// @param index (int)  index of the directory entry.
/// @param buff (Directory_Entry_Block *) buffer to save the Directory_Entry_Block structure
void get_directory_entry_block(unsigned long index, Directory_Entry_Block *buff)
{

    // Compute the offsets
    unsigned long dir_entry_block_number = (unsigned long)floor(
        FIRST_DIR_ENTRY_BLOCK +
        (index / DIR_ENTRIES_PER_BLOCK)); // this found the macro block number

    unsigned long dir_entry_block_offset = (unsigned long)floor(
        index - (dir_entry_block_number * DIR_ENTRIES_PER_BLOCK)); // this finds the offset of the micro block from the macro block number

    // read the blocks
    uint8_t *block_buffer = malloc(SOFTWARE_DISK_BLOCK_SIZE);
    read_sd_block(block_buffer, dir_entry_block_number); // TODO: Check if errors are returned while reading the data

    // Some whiteboard stuff.....
    // We have a 4096-byte block
    // We only need the data starting
    // from
    //      byte dir_entry_block_offset * (SOFTWARE_DISK_BLOCK_SIZE/DIR_ENTRIES_PER_BLOCK)
    // to
    //      byte (dir_entry_block_offset * (SOFTWARE_DISK_BLOCK_SIZE/DIR_ENTRIES_PER_BLOCK)) + (SOFTWARE_DISK_BLOCK_SIZE/DIR_ENTRIES_PER_BLOCK)
    // Time to create a new #define variable DIR_ENTRY_BLOCK_SIZE with value (SOFTWARE_DISK_BLOCK_SIZE/DIR_ENTRIES_PER_BLOCK)

    memcmp(buff, block_buffer + (dir_entry_block_offset * DIR_ENTRY_BLOCK_SIZE), DIR_ENTRY_BLOCK_SIZE);

    free(block_buffer);
}

/// @brief Returns an Inode_Block given an index
/// @param index (int )  index of the Inode.
/// @param buff (Inode_Block *) buffer to save the Inode_Block structure
void get_inode_block(int index, Inode_Block *buff)
{

    // Compute the offsets
    unsigned long inode_block_number = (unsigned long)floor(
        FIRST_INODE_BLOCK +
        (index / INODES_PER_BLOCK)); // this found the macro block number

    unsigned long inode_block_offset = (unsigned long)floor(
        index - (inode_block_number * INODES_PER_BLOCK)); // this finds the offset of the micro block from the macro block number

    // read the blocks
    uint8_t *block_buffer = malloc(SOFTWARE_DISK_BLOCK_SIZE);
    read_sd_block(block_buffer, inode_block_number); // TODO: Check if errors are returned while reading the data

    // Some whiteboard stuff.....
    // We have a 4096-byte block
    // We only need the data starting
    // from
    //      byte inode_block_offset * (SOFTWARE_DISK_BLOCK_SIZE/INODES_PER_BLOCK)
    // to
    //      byte (inode_block_offset * (SOFTWARE_DISK_BLOCK_SIZE/INODES_PER_BLOCK)) + (SOFTWARE_DISK_BLOCK_SIZE/INODES_PER_BLOCK)
    // Time to create a new #define variable INODE_BLOCK_SIZE with value (SOFTWARE_DISK_BLOCK_SIZE/INODES_PER_BLOCK)

    memcmp(buff, block_buffer + (inode_block_offset * INODE_BLOCK_SIZE), INODE_BLOCK_SIZE);
    free(block_buffer);
}

////@brief sets all the data in a data block to 0s
////@param index (uint16_t) the block to hard delete
////@return 1 on success and -1 on failure
int hard_delete(uint16_t index)
{
    if (index < 70)
    {
        printf("Error! Cannot hard delete important memory sections!");
        return -1;
    }

    uint16_t *del_block_buffer = malloc(SOFTWARE_DISK_BLOCK_SIZE);
    memset(del_block_buffer, 0, sizeof(SOFTWARE_DISK_BLOCK_SIZE)); // see the comment thread on left
    write_sd_block(del_block_buffer, index);
    free(del_block_buffer);

    return 1;
}

/**
 * @brief Checks if the filesystem has been initialized correctly.
 *
 *  TODO: Check if this function is doing correctly what is asked
 *        according to the assignment description.
 *
 * @return int return 0 on error else 1.
 */
int check_structure_alignment()
{

    printf("\nBegnning Filesystem Structure Alignment check...\n");
    int return_value;

    printf("\n\tChecking Free Data Blocks bitmap block...\n");
    // Check free data block bitmap
    uint8_t *free_data_bitmap = malloc(sizeof(uint8_t) * SOFTWARE_DISK_BLOCK_SIZE);

    return_value = read_sd_block(free_data_bitmap, DATA_BITMAP_BLOCK);

    if (return_value == 0)
    {
        printf("\tFailed: Free Data Blocks bitmap block alignment check failed.\n");
        sd_print_error();
        return 0;
    }

    for (uint32_t i = 0; i < FIRST_DATA_BLOCK; i++)
    {
        if (free_data_bitmap[i] != 0)
        {
            printf("\tFailed: Free Data Blocks bitmap block alignment check failed.\n");
            fs_print_error();
            return 0;
        };
    }
    for (uint32_t i = FIRST_DATA_BLOCK; i < SOFTWARE_DISK_BLOCK_SIZE; i++)
    {
        if (free_data_bitmap[i] != 1)
        {
            printf("\n\tFailed: Free Data Blocks bitmap block alignment check failed.\n");
            fs_print_error();
            return 0;
        };
    }
    printf("\tPassed: Free Data Blocks bitmap block\n");

    printf("\n\tChecking Free Inodes bitmap block...\n");
    uint8_t *free_inode_bitmap = malloc(sizeof(uint8_t) * SOFTWARE_DISK_BLOCK_SIZE);

    return_value = read_sd_block(free_inode_bitmap, INODE_BITMAP_BLOCK);
    if (return_value == 0)
    {
        printf("\tFailed: Free Inodes bitmap block alignment check failed.\n");
        sd_print_error();
        return 0;
    }

    for (uint32_t i = 0; i < SOFTWARE_DISK_BLOCK_SIZE; i++)
    {
        if (free_inode_bitmap[i] != 1)
        {
            printf("\tFailed: Free Inodes bitmap block alignment check failed.\n");
            fs_print_error();
            return 0;
        };
    }

    printf("\tPassed: Free Inodes bitmap block\n");

    printf("\n\tChecking rest of the filesystem blocks...\n");
    for (uint32_t i = FIRST_INODE_BLOCK; i < SOFTWARE_DISK_BLOCK_SIZE; i++)
    {

        uint8_t *buffer = malloc(sizeof(uint8_t) * SOFTWARE_DISK_BLOCK_SIZE);

        return_value = read_sd_block(buffer, i);
        if (return_value == 0)
        {
            printf("\tFailed: block alignment check failed.\n");
            sd_print_error();
            return 0;
        }

        for (uint32_t i = 0; i < SOFTWARE_DISK_BLOCK_SIZE; i++)
        {
            if (buffer[i] != 0)
            {
                printf("\tFailed: block alignment check failed.\n");
                fs_print_error();
                return 0;
            };
        }
    }

    printf("\nPassed: All the blocks are correctly aligned.\n");

    return 1;
}

void fs_print_error(void)
{

    switch (FSERROR)
    {
    case FS_NONE:
        printf("FS: No error.\n");
        break;
    case FS_OUT_OF_SPACE:
        printf("FS: The operation caused the software disk to fill up.\n");
        break;
    case FS_FILE_NOT_OPEN:
        printf("FS: Attempted read/write/close/etc. on file that isn't open.\n");
        break;
    case FS_FILE_OPEN:
        printf("FS: file is already open. Concurrent opens are not supported and neither is deleting a file that is open.\n");
        break;
    case FS_FILE_NOT_FOUND:
        printf("FS: Attempted open or delete of file that does't exist.\n");
        break;
    case FS_FILE_READ_ONLY:
        printf("FS: Attempted write to file opened for READ_ONLY.\n");
        break;
    case FS_FILE_ALREADY_EXISTS:
        printf("FS: Attempted creation of file with existing name.\n");
        break;

    case FS_EXCEEDS_MAX_FILE_SIZE:
        printf("FS: Seek or Write would exceed max file size.\n");
        break;
    case FS_ILLEGAL_FILENAME:
        printf("FS: Filename begins with a null character.\n");
        break;
    case FS_IO_ERROR:
        printf("FS: something really bad happened.\n");
        break;
    default:
        printf("FS: Unknown error code %d.\n", FSERROR);
    }
}

File open_file(char *name, FileMode mode)
{

    /**
     * TODO:
     *
     *  1. Check if the name and mode is valid
     *      1.a it should not begin with a null
     *      1.b the file name should not exceed 508 bytes.
     *      1.c mode should be one defined in
     *          the FileMode enum
     *
     *  2. Search through the Dir_Entry Blocks to
     *      locate the file inode.
     *
     *  3. Check the inode mode against mode.
     *     It should be NULL as the file isn't opened yet.
     *
     *  4. Set the mode and return the pointer to
     *      the inode. This pointer will be of type
     *      FileInternals.
     *
     */

    // Checking the function arguments
    FSERROR = FS_NONE;

    int file_name_length = strlen(name);

    if (file_name_length > MAX_FILENAME_SIZE || name[0] == '\0')
    {
        FSERROR = FS_ILLEGAL_FILENAME;
        return NULL;
    }

    if (mode != READ_ONLY && mode != READ_WRITE)
    {
        FSERROR = FS_IO_ERROR;
        return NULL;
    }

    // This is a temporary structure to hold our directory entry.
    Directory_Entry_Block *dir_entry = malloc(sizeof(Directory_Entry_Block));

    // Search through the directory entries to locate the file.
    for (int dir_entry_index = 0; dir_entry_index < MAX_FILES; dir_entry_index++)
    {

        get_directory_entry_block(dir_entry_index, dir_entry);

        if (strcmp(dir_entry->file_name, name) == 0)
        {
            break;
        }
    }

    Inode_Block *inode = malloc(sizeof(Inode_Block));
    get_inode_block(dir_entry->inode_pointer, inode);

    File file = malloc(sizeof(FileInternals));
    file->inode = inode;
    file->offset = 0;
    file->inode->mode = mode;

    return file;
}

File create_file(char *name)
{

    /**
     * TODO:
     *   1. check the input (if it is valid)
     *       1.a first charracter must not
     *           be the NULL character
     *       1.b The length should not be more
     *           than 507 bytes
     *       1.c It should be unique and not
     *           be used by any other file.
     *
     *  2. Allocate and write a Inode_Block
     *     Structure to the disk.
     *
     *  3. Update the Free Inode Bitmap
     *
     *  4. Update the free datablock bitmap
     *
     *  5. Allocate and write a Directory_Entry_Block
     *     Structure to the disk
     *
     *      Note: Rollback any changes from step 2.
     *           if any of the steps fail.
     *
     *  6. Call open_file to return the file handle.
     */

    // Checking the function arguments
    FSERROR = FS_NONE;

    int file_name_length = strlen(name);

    if (file_name_length > MAX_FILENAME_SIZE || name[0] == '\0')
    {
        FSERROR = FS_ILLEGAL_FILENAME;
        return NULL;
    }

    // This is a temporary structure to hold our directory entry.
    Directory_Entry_Block *dir_entry = malloc(sizeof(Directory_Entry_Block));

    // Check if the file name is unique
    for (int dir_entry_index = 0; dir_entry_index < MAX_FILES; dir_entry_index++)
    {
        get_directory_entry_block(dir_entry_index, dir_entry);

        if (strcmp(dir_entry->file_name, name) == 0)
        {
            FSERROR = FS_FILE_ALREADY_EXISTS;
            return NULL;
        }
    }

    // No file with file_name `name` exists. So, we go ahead and create a new file

    // TODO: Do we need to allocate a block when a file is created?

    int free_inode_index = get_free_index(INODE_BITMAP_BLOCK);
    int free_indirect_block_index = get_free_index(DATA_BITMAP_BLOCK);

    if (free_inode_index < 0 ||
        free_indirect_block_index < 0)
    {
        FSERROR = FS_OUT_OF_SPACE;
        return NULL;
    }

    Inode_Block *inode = malloc(sizeof(Inode_Block));

    inode->indirect_pointers = free_indirect_block_index;
    inode->file_size = 0;
    inode->mode = NONE;

    memcpy(dir_entry->file_name, name, strlen(name));
    dir_entry->file_name_size = strlen(name);
    dir_entry->inode_pointer = free_inode_index;

    // TODO: TO check for errors when updating the blocks
    update_inode_block(free_inode_index, inode);

    // TODO: TO check for errors when updating the blocks
    update_directory_entry_block(free_inode_index, dir_entry);

    return open_file(name, READ_WRITE);
}

unsigned long read_file(File file, void *buf, unsigned long numbytes)
{

    /**
     * TODO:
     *
     *      1. Check the arguments
     *          1.a numbytes + offset should be greater than 0
     *
     *      2. Read the data into the buffer
     *
     *      3. return 0     on error,
     *         bytes_read   if numbytes + offset is greater than MAX_FILE_SIZE
     *         1            otherwise.
     */

    FSERROR = FS_NONE;

    int number_of_bytes_read = 0;

    if (file->inode->mode == NONE)
    {
        FSERROR = FS_FILE_NOT_OPEN;
        return number_of_bytes_read;
    }

    if (file->offset + ((double)numbytes / SOFTWARE_DISK_BLOCK_SIZE) < 0)
    {
        return number_of_bytes_read;
    }

    // uint8_t *buffer = malloc(sizeof(uint_8))

     uint16_t *indirect_pointers;
    
    int number_of_blocks_to_read = ceil((double)numbytes / SOFTWARE_DISK_BLOCK_SIZE);

    if (number_of_blocks_to_read >= NUM_DIRECT_INODE_BLOCKS) {
        
        indirect_pointers = malloc(sizeof(NUM_SINGLE_INDIRECT_BLOCKS));
        
        read_sd_block(indirect_pointers, file->inode->indirect_pointers);
    }

    // TODO: Test for end of file and error handling

    for(int block_index = 0; block_index < number_of_blocks_to_read && block_index < 2061; block_index++) {
        
        uint8_t *buffer = malloc(SOFTWARE_DISK_BLOCK_SIZE);

        if (block_index < NUM_DIRECT_INODE_BLOCKS) {
            read_sd_block(buffer, file->inode->direct_pointers[block_index]);
        } else {
            read_sd_block(buffer, indirect_pointers[block_index - NUM_DIRECT_INODE_BLOCKS]);
        }

        if (block_index == number_of_blocks_to_read - 1) {

            // memcpy(buf + number_of_bytes_read, buffer, numbytes - number_of_bytes_read);
            // number_of_bytes_read += numbytes - number_of_bytes_read;

            memcpy(buf, buffer, numbytes - number_of_bytes_read);
            number_of_bytes_read += numbytes - number_of_bytes_read;
    
        } else {

            memcpy(buf, buffer, SOFTWARE_DISK_BLOCK_SIZE);
            number_of_bytes_read += SOFTWARE_DISK_BLOCK_SIZE; 
            buf += number_of_bytes_read;
        }

    }

    return number_of_bytes_read;


}

unsigned long write_file(File file, void *buf, unsigned long numbytes)
{

    /**
     * TODO:
     *      1. Check the arguments
     *          1.a numbytes + offset should be greater than 0
     *
     *      2. Check if the mode is READ_WRITE
     *
     *      3. Compute the number of bytes to write.
     *         i.e. Make sure that numbytes + offset is less than FILE_SIZE.
     *
     *      4. If the numbytes + offset is less than FILE_SIZE,
     *         write the buffer to file and return 1
     *
     *      5. If the numbytes + offset is greater than FILE_SIZE.
     *          write the bytes upto MAX_FILE_SIZE and return the number of
     *          bytes written and set the FS_OUT_OF_SPACE error.
     *
     */

    FSERROR = FS_NONE;

    unsigned long number_of_bytes_written = 0;

    if (file->inode->mode == NONE)
    {
        FSERROR = FS_FILE_NOT_OPEN;
        return number_of_bytes_written;
    }

    if (file->inode->mode != READ_WRITE)
    {
        FSERROR = FS_FILE_READ_ONLY;
        return number_of_bytes_written;
    }

    if (file->offset + (numbytes / SOFTWARE_DISK_BLOCK_SIZE) < 0)
    {
        return number_of_bytes_written;
    }


   /**
    * Some whiteboarding....
    * 
    *   current offset is somewhere in the middle of the block
    *   Assuming that numbytes is positive.
    * 
    * 
    *   SOFTWARE_DISK_BLOCK_SIZE - offset => number of bytes that can be written
    *   in current block
    * 
    *   => This implies that 
    *      we require (numbytes - (SOFTWARE_DISK_BLOCK_SIZE - offset))/SOFTWARE_DISK_BLOCK_SIZE
    *      additional blocks.
    * 
    *   Next we need figure out whether we require indirect blocks or not.
    *   Scratch that! Just assume that we require indirect block at all times.
    * 
    * 
    *   next we loop over for
    * 
    *   (numbytes - (SOFTWARE_DISK_BLOCK_SIZE - offset))/SOFTWARE_DISK_BLOCK_SIZE +
    *   ceil(SOFTWARE_DISK_BLOCK_SIZE - offset)
    * 
    *   blocks
    * 
   */

    uint16_t *indirect_pointers = malloc(sizeof(NUM_SINGLE_INDIRECT_BLOCKS));
    read_sd_block(indirect_pointers, file->inode->indirect_pointers);


    int current_block_index = floor((double)file->offset / SOFTWARE_DISK_BLOCK_SIZE);

    
    unsigned long current_block_size = file->offset - (current_block_index * SOFTWARE_DISK_BLOCK_SIZE);
    unsigned long free_bytes_in_current_block = SOFTWARE_DISK_BLOCK_SIZE - (file->offset - (current_block_index * SOFTWARE_DISK_BLOCK_SIZE));


    uint8_t *current_block_buffer = malloc(SOFTWARE_DISK_BLOCK_SIZE);   

    if(free_bytes_in_current_block > 0 && free_bytes_in_current_block < SOFTWARE_DISK_BLOCK_SIZE) {

        read_sd_block(current_block_buffer, current_block_index);
        memcpy(current_block_buffer + current_block_size, buf, free_bytes_in_current_block);
        write_sd_block(current_block_buffer, current_block_index);

        number_of_bytes_written += free_bytes_in_current_block;
        numbytes -= free_bytes_in_current_block;
         file->offset += free_bytes_in_current_block;
        current_block_index += 1;
        buf += free_bytes_in_current_block;
    }


    double required_blocks;
    if (numbytes > 0) {

        required_blocks = ceil((double)numbytes/SOFTWARE_DISK_BLOCK_SIZE);
    } else {
        required_blocks = 0;
    }

    // TODO: Error checks
    for(int block_index = current_block_index; block_index < current_block_index + required_blocks && block_index < 2061; block_index++) {

        int free_data_block_index = get_free_index(DATA_BITMAP_BLOCK);

        if (free_data_block_index == -1){
            FSERROR = FS_OUT_OF_SPACE;
            return number_of_bytes_written;
        }

        if (numbytes >= SOFTWARE_DISK_BLOCK_SIZE) {
            memcpy(current_block_buffer, buf, SOFTWARE_DISK_BLOCK_SIZE);
            write_sd_block(current_block_buffer, free_data_block_index);
            number_of_bytes_written += SOFTWARE_DISK_BLOCK_SIZE;
            numbytes -= SOFTWARE_DISK_BLOCK_SIZE;
            file->offset += SOFTWARE_DISK_BLOCK_SIZE;
            buf += SOFTWARE_DISK_BLOCK_SIZE;

        } else {
            memcpy(current_block_buffer, buf, numbytes);
            write_sd_block(current_block_buffer, free_data_block_index);

            number_of_bytes_written += SOFTWARE_DISK_BLOCK_SIZE;
            numbytes -= numbytes;
            file->offset += numbytes;
            buf += numbytes;
        }

        

        if (block_index < NUM_DIRECT_INODE_BLOCKS) {

            file->inode->direct_pointers[block_index] = free_data_block_index;
        } else {

            indirect_pointers[block_index - NUM_DIRECT_INODE_BLOCKS] = free_data_block_index;
        }

        unset_bitmap(DATA_BITMAP_BLOCK, free_data_block_index);


        
    }

    write_sd_block(indirect_pointers, file->inode->indirect_pointers);
    int c = get_free_index(DATA_BITMAP_BLOCK);
    return number_of_bytes_written;
    
}

void close_file(File file)
{
    /**
     * TODO:
     *
     *  1. Check the file mode. It should either
     *     be READ_ONLY, READ_WRITE signifying that
     *     it is open.
        ./
     *
     *  2. If the check passes, set the mode
     *      to NONW.
        ./
     *
     *
     *  Note: Is NULL the correct way/ the ideal way
     *        to signify that a file is open or not?
     *
     *        Can we do something better?
     *        Like mode is 1 for READ_ONLY
     *                     2 forREAD_WRITE
     *                     0 otherwise
     *
     */

     if(file->inode->mode == READ_WRITE || file->inode->mode == READ_ONLY)
     {
        file->inode->mode = NONE;
     }

}

int file_exists(char *name)
{

    /**
     * TODO:
     *
     *  1. Check the input. It should be valid.
     *     i.e., it should not start with null character
     *     and it's length should not exceed 508 bytes.
     *
     *  2. Scan through the DIR_Entry blocks and
     *     return 1 if we find a match else return 0.
     */

    if (name[0] == '\0')
    {
        printf("Error! Must input file name to delete!");
        return -1;
    }

    short length = 1;
    while (name[length] != '\0')
    {
        if (length == 508)
        {
            printf("Error! File name too long!"); // 012345\n
            return -1;                            // 1234567
        }
        length++;
    }

    // scan throug Dir_Entry blocks to find the file
    Directory_Entry_Block *del_file;
    int index = 0;
    get_directory_entry_block(index, del_file);
    while (strcpy(name, del_file->file_name))
    {
        index = index + 1;
        if(index > 59)
        {
            return 0;
        }
        get_directory_entry_block(index, del_file);
    }
    return 1;
}

unsigned long file_length(File file)
{

    /**
     * TODO:
     *
     *  return the file_size value ,
     *  typecast it into unsigned long and return it.
     */
    
    return (unsigned long) file->inode->file_size ;
}

int delete_file(char *name)
{
    /**
     * TODO:
     *
     *    1. Check the name
     *      1.a it should not begin with a null
     *      1.b the file name should not exceed 508 bytes.
     *      ./
     *    2. Scan through the Dir_Entry blocks and find the entry
     *      ./
     *    3. If the file is not open, delete the Inode_Block Entry.
     *         3.a  file is not open: the mode in Inode_Block should be 0.
     *         3.b  to purge the Inode_Block Entry, we can either overite
     *              the Inode location with 0 or we can simply update the
     *              INODE_BITMAP_BLOCK.
     *         3.c  Update the inode bitmap INODE_BITMAP_BLOCK
            ./
     *
     *     4. Purge the Directory_Entry_Block found in step 2.
     *          4.a Again we can either overwrite the Directory_Entry_Block location
     *              with 0 or just update the DATA_BITMAP_BLOCK
     *
     *          4.b Update the DATA_BITMAP_BLOCK
            ./
     *
     *
     *
     */

    // check name
    if (name[0] == '\0')
    {
        printf("Error! Must input file name to delete!");
        return -1;
    }

    short length = 1;
    while (name[length] != '\0')
    {
        if (length == 508)
        {
            printf("Error! File name too long!"); // 012345\n
            return -1;                            // 1234567
        }
        length++;
    }

    // scan throug Dir_Entry blocks to find the file
    Directory_Entry_Block *del_file = malloc(sizeof(Directory_Entry_Block));
    int index = 0;
    get_directory_entry_block(index, del_file);
    while (strcpy(name, del_file->file_name))
    {
        index = index + 1;
        get_directory_entry_block(index, del_file);
    }
    
    // 
    //check if file is open
    // Inode_Block *del_inode = malloc(sizeof(Inode_Block));

    Inode_Block *del_inode = malloc(sizeof(Inode_Block));
    
    // get_inode_block(del_inode, (void*) (long) del_file->inode_pointer);

    get_inode_block(del_file->inode_pointer, del_inode);


    if (del_inode->mode == READ_ONLY || del_inode->mode == READ_WRITE)
    {
        printf("ERROR! File is open! Close file to delete.");
        return -1;
    }

    //begin deletion
    int counter;
    for (counter = 0; counter < 13; counter++)
    {
        hard_delete(del_inode->direct_pointers[counter]);
        unset_bitmap(DATA_BITMAP_BLOCK, del_inode->direct_pointers[counter]);
    }

    Indirect_Block *del_indirect = (void*) (long) del_inode->indirect_pointers;
    for (counter = 0; counter < NUM_SINGLE_INDIRECT_BLOCKS; counter++)
    {
        hard_delete(del_indirect->cheap_pointers[counter]);
        unset_bitmap(DATA_BITMAP_BLOCK, del_indirect->cheap_pointers[counter]);
    }
    hard_delete(del_inode->indirect_pointers);

    uint16_t *burn_inode = malloc(sizeof(INODE_BLOCK_SIZE));
    memset(burn_inode, 0, sizeof(INODE_BLOCK_SIZE)); // see the comment thread on left
    write_sd_block(burn_inode, del_file->inode_pointer);
    free(burn_inode);

    uint16_t *burn_dir = malloc(sizeof(DIR_ENTRY_BLOCK_SIZE));
    memset(burn_dir, 0, sizeof(DIR_ENTRY_BLOCK_SIZE)); // see the comment thread on left
    write_sd_block(burn_dir, index);
    free(burn_dir);

    unset_bitmap(DATA_BITMAP_BLOCK, del_inode->indirect_pointers);
    unset_bitmap(INODE_BITMAP_BLOCK,index);
    

    return 1;
}


// sets current position in file to 'bytepos', always relative to the
// beginning of file.  Seeks past the current end of file should
// extend the file. Returns 1 on success and 0 on failure.  Always
// sets 'fserror' global.

int seek_file(File file, unsigned long bytepos)
{

    /**
     * TODO:
     *
     *      1. Check the input arguments
     *          1.a bytepos should be greater than 0 but less than
     *          2061 (13 direct blocks + 2048 indirect blocks)
     *
     *      2. Increment the offset field in File.
     *
     *          Note: if the seek is past the current file size
     *              i.e. if it is greater than `file_size` field in the Inode block,
     *              we update the `file_size` field.
     *
     *      3. return 1 on success 0 otherwise.
     */

     if(bytepos >2060)
        return 0;
    
    file->offset=bytepos;

    if(bytepos > file_length(file))
    {
        file->inode->file_size = bytepos;
    }
    return 1;
}

// int main()
// {

//     printf("SIZE of Inode_Block: %d\n", sizeof(Inode_Block));
//     printf("SIZE of Directory_Entry_Block: %d\n", sizeof(Directory_Entry_Block));
// }