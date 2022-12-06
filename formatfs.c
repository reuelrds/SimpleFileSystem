#include <stdlib.h>
#include <strings.h>

#include "softwaredisk.h"
#include "filesystem.h"

/**
 * @brief Initialize the File system
 *
 *  Initializes the two bitmaps.
 *  Tghe bit is set to 1 if free and 0 if used/occupied.
 * 
 *  Note: The bitmaps are not exactly bits but each "bit"
 *        map is a 8-bit or 1 byte long.
 *        This is because, both the bitmaps occupy an 
 *        entire block and are of size SOFTWARE_DISK_BLOCK_SIZE 
 *        So, instead of having actual bits we can
 *        have 8-bit long "bits" which makes dealing with 
 *        them a bit easier.
 * 
 *  
 *  All the bits in the Free Inode Bitmap are 1 as was 
 *  initially there are no inodes.
 *
 *  The first FIRST_DATA_BLOCK entries of the free data block are special
 *  and are used by the filesystem to track information. Thys they are set
 *  to 0 signifying that the blocks are occupied. The rest of the blocks
 *  are set to 1.
 *
 * @return int return -1 on error else 0.
 */
int format_disk()
{
    int return_status;

    // Initialize the software disk
    return_status = init_software_disk();

    if (return_status == 0)
    {
        sd_print_error();
        return -1;
    }

    // Set up the Free Inode Bitmaps
    uint8_t *free_inode_bitmap = malloc(sizeof(uint8_t) * SOFTWARE_DISK_BLOCK_SIZE);
    memset(free_inode_bitmap, 1, SOFTWARE_DISK_BLOCK_SIZE);


    // Set up the Free Data Block Bitmaps
    uint8_t *free_data_bitmap = malloc(sizeof(uint8_t) * SOFTWARE_DISK_BLOCK_SIZE);

    for (uint32_t i = 0; i < FIRST_DATA_BLOCK; i++)
    {
        free_data_bitmap[i] = 0;
    }
    for (uint32_t i = FIRST_DATA_BLOCK; i < SOFTWARE_DISK_BLOCK_SIZE; i++)
    {
        free_data_bitmap[i] = 1;
    }


    // Flush the bitmaps to the disk
    return_status = write_sd_block(free_inode_bitmap, INODE_BITMAP_BLOCK);

    if (return_status == 0)
    {
        sd_print_error();
        return -1;
    }


    return_status = write_sd_block(free_data_bitmap, DATA_BITMAP_BLOCK);
    if (return_status == 0)
    {
        sd_print_error();
        return -1;
    }


    // Check alignment
    return_status = check_structure_alignment();


    return return_status;
}

int main(int argc, char const *argv[])
{
    format_disk();

    return 0;
}
