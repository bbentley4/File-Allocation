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
    //total_sectors = T //data_sectors = D
    // T = D + ((D+1)/512)
    // T - D = (D+1)/512
    // T*512 - D*512 = D+1
    // T*512-1 = 513D
    // ((T*512)-1)/513 = D
    int data_sectors = ((total_sectors*512)-1)/513;
    printf("DS: %i\nBKS: %i", data_sectors, total_sectors-data_sectors);
    return data_sectors;
}


int main(int argc, char **argv)
{
    // if (argc != 4 && argc != 6)
    // {
    //     printf("Usage: ");
    // }

    void* disk_ptr = jdisk_attach(argv[1]);
    int total_sectors =  CalcTotalSectors(disk_ptr);
    int data_sectors =  CalculateDataSectors(total_sectors);
    return 0;
}
