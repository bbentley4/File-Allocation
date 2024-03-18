/*
    Author: Brenna Bentley
    Date: 3/08/2024
    Date of Last Edit: 3/12/2024

    Description: This program is emmulating im- and exporting to/from disks using .jdisk files 
                 instead of actual disks. The main focus is the manipulation of the File Allocation
                 Table, as the read and write operations have been written for me by Dr. James Plank 
                 in jdisk.c. 
    For more information look at the html pages in this directory written by Dr. Plank.

    Sources: 
        -I saw Dr. Plank's Usage fx in jdisk_test and decided to implement it as well
*/
// TODO remove the /n from the usage message when printing out messages
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/mman.h>
#include <string.h>
#include <assert.h> //TODO maybe remove later?
#include <fcntl.h>
#include "jdisk.h"

#define LINKS_PER_PAGE (JDISK_SECTOR_SIZE/(sizeof(short)))
typedef struct {
    void* diskptr;
    int total; // total sectors  
    int data; // data sectors
    int fat; // fat sectors
} DiskStats;

// Prints standard usage error w/ opt to add specifics by passing argument
void UsageError (char* addtl)
{
    printf("usage: FATRW diskfile import input-file\n");
    printf("       FATRW diskfile export starting-block output-file\n");  
    if (addtl != NULL)  printf("%s\n", addtl);     
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
    // T = total sectors & D = data sectors & S = FAT sectors & L = links per page
    // T = D + S
    // S = (D+1)/L
    // T = D + ((D+1)/L)
    // T - D = (D+1)/L
    // T*L - D*L = D+1
    // T*L-1 = (L+1)D
    // ((T*L)-1)/(L+1) = D
    int dataSectors = ((totalSectors*LINKS_PER_PAGE)-1)/(LINKS_PER_PAGE+1);
    return dataSectors;
}

// Set the variables assoc w/ the disk: ptr, total sectors, data sectors, and FAT sectors
void SetDiskValues (DiskStats* ds, char* file)
{
    ds->diskptr = jdisk_attach(file);
    if (ds->diskptr == NULL)
    {
        UsageError((char*) "Could not attach diskfile.\n");
        exit(EXIT_FAILURE);
    } 
    ds->total = CalcTotalSectors(ds->diskptr);
    ds->data = CalcDataSectors(ds->total);
    ds->fat = ds->total - ds->data;
    //printf("diskptr = %p\ntotal: %d\ndata: %d\nfat: %d\n", ds->diskptr, ds->total, ds->data, ds->fat);
}

void ImportHandler (char* file, DiskStats ds)
{

} 

void ExportHandler (char* file, DiskStats ds, int sector)
{

}

// Prints the links in 1 block of the FAT stored in buff for error checking.
void PrintFAT (void* buff, DiskStats ds, int cur_block)
{
    short* links = (short*)buff;
    if (cur_block < ds.fat - 1)
    {
        for (int i = 0; i < LINKS_PER_PAGE; i++)
        {
            printf("Link %d: %d\n", i + (int)(LINKS_PER_PAGE * cur_block), links[i]);
        }
    }
    else 
    {
        for (int i = 0; i < ds.total%LINKS_PER_PAGE; i++)
        {
            printf("Link %d: %d\n", i + (int)(LINKS_PER_PAGE * cur_block), links[i]);
        }
    }
}

int main(int argc, char** argv)
{
    //Variable declaration 
    DiskStats ds;
    void* FAT_buff = malloc(JDISK_SECTOR_SIZE);
    if (FAT_buff == NULL) 
    {
        perror("Error allocating memory for FAT buffer");
        exit(EXIT_FAILURE);
    }
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
            int fd = open(argv[4], O_WRONLY | O_CREAT, 0666);
        }
    }
    else if (argc == 2)
    {
        SetDiskValues(&ds, argv[1]);
        printf("diskptr = %p\ntotal: %d\ndata: %d\nfat: %d\n", ds.diskptr, ds.total, ds.data, ds.fat);
        for (int i = 0; i < ds.fat; i++)
        {
            jdisk_read(ds.diskptr, i, FAT_buff);
            PrintFAT (FAT_buff, ds, i);
        }
    }
    else     UsageError(NULL);

    /* -------------------------------------------------------------------------- */
    /*                                  Clean Up                                  */
    /* -------------------------------------------------------------------------- */
    free(FAT_buff);
    return 0;
}
