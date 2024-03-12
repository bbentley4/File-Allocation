/*
    Author: Brenna Bentley
    Date: 3/11/2024
    Date of Last Edit: 3/11/2024

    Description: This program is emmulating im- and exporting from disks using .jdisk files 
                 instead of actual disks. The main focus is the manipulation of the File Allocation
                 Table, as the read and write operations have been written for me by Dr. James Plank 
                 in jdisk.c. 
    For more information:  http://web.eecs.utk.edu/~jplank/plank/classes/cs494/494/labs/Lab-2-FAT/

    Sources: 
        -I saw Dr. Plank's jdisk_test and decided to implement it as well
*/
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/mman.h>
#include <string.h>
#include <assert.h>
#include "jdisk.h"

typedef struct {
    void* diskptr;
    int total; // total sectors  
    int data; // data sectors
    int fat; // fat sectors
} DiskStats;

// Prints standard usage error w/ opt to add specifics by passing argument
void UsageError (char* addtl)
{
    printf("Usage: FATRW diskfile import input-file\n");
    printf("FATRW diskfile export starting-block output-file\n");  
    if (addtl != NULL)  printf("\t%s\n", addtl);     
}

// Uses jdisk_size & JDISK_SECTOR_SIZE to return the total number of sectors 
int CalcTotalSectors (void* diskptr)
{
    unsigned long diskSize = jdisk_size(diskptr);
    int sectors = diskSize / JDISK_SECTOR_SIZE;
    return sectors;
}

// Returns the total number of data sectors (sectors not taken by FAT)
int CalcDataSectors (int totalSectors)
{
    // T = total sectors & D = data sectors & S = FAT sectors
    // T = D + S
    // S = (D+1)/512
    // T = D + ((D+1)/512)
    // T - D = (D+1)/512
    // T*512 - D*512 = D+1
    // T*512-1 = 513D
    // ((T*512)-1)/513 = D
    int dataSectors = ((totalSectors*512)-1)/513;
    return dataSectors;
}

// Prints the links in 1 block of the FAT stored in buff for error checking.
void PrintFAT (void* buff, int dataSectors)
{
    short* links = (short*)buff;
    for (int i = 0; i <= dataSectors; i++)
    {
        printf("Link %d: %d\n", i, links[i]);
    }
}

// Set the variables assoc w/ the disk: ptr, total sectors, data sectors, and FAT sectors
void SetDiskValues (DiskStats *ds, char* file)
{
    // TODO prevent segfault?
    ds->diskptr = jdisk_attach(file);
    if (ds->diskptr == NULL)
    {
        UsageError((char*) "Could not attach diskfile.\n");
        exit(EXIT_FAILURE);
    } 
    ds->total = CalcTotalSectors(ds->diskptr);
    ds->data = CalcDataSectors(ds->total);
    ds->fat = ds->total - ds->data;
    printf("diskptr = %p\ntotal: %d\ndata: %d\nfat: %d\n", ds->diskptr, ds->total, ds->data, ds->fat);
}

void ImportHandler (char* file, DiskStats ds)
{

} 

void ExportHandler (char* file, DiskStats ds, int sector)
{

}

int main(int argc, char** argv)
{
    //Variable declaration 
    DiskStats ds;
    void* FAT_buff = malloc(ds.fat * JDISK_SECTOR_SIZE);

    // Error check argument count
    if (argc == 4 && strcmp(argv[2], "import") == 0)
    {
        // Attach/open disk & error check
        SetDiskValues(&ds, argv[1]);
    }
    else if (argc == 5 && strcmp(argv[2], "export") == 0)
    {
        // Attach/open disk & error check
        SetDiskValues(&ds, argv[1]);

        int lba;
        if (sscanf(argv[3], "%d", &lba) == 0) UsageError((char *) "LBA must be an integer value.\n");
        
        else if (lba < ds.fat)
        {
            printf("Error in Export: LBA is not for a data sector.\n");
            return -1;
        }
        else if (lba > ds.total + 1)
        {
            printf("Error in Export: LBA too big\n");
            return -1;
        }
        else // starting-block is valid
        {   
            //TODO fopen/create here for export file. 
            //TODO is truncate correct?
            FILE* file_ptr = fopen(argv[4], "w");
            if (file_ptr == NULL)
            {
                perror("Error opening file for w");
            }
        }
    }
    else if (argc == 2)
    {
        SetDiskValues(&ds, argv[1]);
        printf("diskptr = %p\ntotal: %d\ndata: %d\nfat: %d\n", ds.diskptr, ds.total, ds.data, ds.fat);
        for (int i = 0; i < ds.fat; i++)
        {
            jdisk_read(ds.diskptr, i, FAT_buff);
            PrintFAT (FAT_buff, ds.data);
        }
    }
    else     UsageError(NULL);

    /* -------------------------------------------------------------------------- */
    /*                                  Clean Up                                  */
    /* -------------------------------------------------------------------------- */
    free(FAT_buff);
    return 0;
}
