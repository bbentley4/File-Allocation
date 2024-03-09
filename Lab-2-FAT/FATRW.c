#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/mman.h>
#include <string.h>
#include "jdisk.h"

int CalcTotalSectors (void* disk_ptr)
{
    unsigned long disk_size = jdisk_size(disk_ptr);
    // TODO Can always assume 1024? - WRITE AN ASSERT
    int sectors = disk_size / 1024;
    return sectors;
}

int CalculateDataSectors (int total_sectors)
{
    // T = total sectors & D = data sectors & S = FAT sectors
    // T = D + S
    // S = (D+1)/512
    // T = D + ((D+1)/512)
    // T - D = (D+1)/512
    // T*512 - D*512 = D+1
    // T*512-1 = 513D
    // ((T*512)-1)/513 = D
    int data_sectors = ((total_sectors*512)-1)/513;
    return data_sectors;
}

int main(int argc, char **argv)
{
    //Variable declaration 
    void* disk_ptr;
    int total_sectors, data_sectors, FAT_sectors;
    int im_ex; // 0 = import, 1 = export
    // Error check argument count
    if (argc != 4 && argc != 6)
    {
        printf("Usage: FATRW diskfile import input-file\nFATRW diskfile export starting-block output-file");
        return -1;
    }
    
    // Attach/open disk & error check
    disk_ptr = jdisk_attach(argv[1]);
    if (disk_ptr == NULL)
    {
        perror("Could not attach diskfile.\n");
        return -1;
    } 
    
    // Check second argument 
    if (strcmp(argv[2], "import") == 0)
    {
        im_ex = 0;
    }
    else if (strcmp(argv[2], "export") == 0)
    {
        im_ex = 1;
    }
    else 
    {
        printf("Usage: FATRW diskfile import input-file\nFATRW diskfile export starting-block output-file");
        return -1;
    }

    //Check third argument 
    if (im_ex == 0)
    {
        
    }
    total_sectors =  CalcTotalSectors(disk_ptr);
    data_sectors =  CalculateDataSectors(total_sectors);
    FAT_sectors = total_sectors - data_sectors;
    return 0;
}
