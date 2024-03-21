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
        - Bitshifting: https://stackoverflow.com/questions/14713102/what-does-and-0xff-do
*/

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/mman.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "jdisk.h"

#define LINKS_PER_PAGE (JDISK_SECTOR_SIZE/(sizeof(short)))
typedef struct {
    void* diskptr;
    int total; // total sectors  
    int data; // data sectors
    int fat; // fat sectors
    unsigned short **FATable;
    int* updated_blocks;
} DiskStats;

void UsageError(char* addtl);
int CalcTotalSectors(void* diskptr);
int CalcDataSectors(int totalSectors);
void SetDiskValues(DiskStats* ds, char* file);
void ImportHandler(char** argvv, DiskStats *ds);
void ExportHandler(char** argvv, DiskStats *ds);
unsigned short GetLink(unsigned short link_index, DiskStats* ds);
void UpdateLink(unsigned short link_index, unsigned short new_value, DiskStats* ds);
void UpdateDiskFAT(DiskStats* ds);
unsigned int LinkToBlock(unsigned short link, int fat);
unsigned short BlocktoLink(unsigned int block, int fat);
void PrintFAT(void* buff, DiskStats ds, int cur_block);

/* -------------------------------------------------------------------------- */
/*                                    MAIN                                    */
/* -------------------------------------------------------------------------- */
int main(int argc, char** argv)
{
    // Variable declaration 
    DiskStats ds;
    // Calloc the FATable from the ds struct
    ds.FATable = (unsigned short **)calloc(ds.total, sizeof(unsigned short *));
    if (ds.FATable == NULL)
    {
        perror("Error allocating memory for FATable");
        free(ds.FATable);
        return -1;
    }
    ds.updated_blocks = (int*)calloc(ds.total, sizeof(int));
    if (ds.updated_blocks == NULL)
    {
        perror("Error allocating memory for updated_blocks");
        free(ds.updated_blocks);
        return -1;
    }
    {
        perror("Error allocating memory for FAT buffer");
        return -1;
    }
    
    // Error check argument count
    if (argc == 4 && strcmp(argv[2], "import") == 0)    ImportHandler(argv, ds);
    else if (argc == 5 && strcmp(argv[2], "export") == 0)   ExportHandler(argv, ds);
    else     UsageError(NULL);

    /* -------------------------------------------------------------------------- */
    /*                                  Clean Up                                  */
    /* -------------------------------------------------------------------------- */
    return 0;
}


// Prints standard usage error w/ opt to add specifics by passing argument
void UsageError (char* addtl)
{
    printf("usage: FATRW diskfile import input-file\n");
    printf("       FATRW diskfile export starting-block output-file\n");  
    if (addtl != NULL)  printf("%s\n", addtl);     
    exit(1);
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

void ImportHandler (char** argvv, DiskStats *ds)
{
    int fd = open(argvv[4], O_RDONLY);
    if (fd == -1)
    {
        perror("Error opening input file for reading");
        exit(-1);
    }

    char buffer[1024];

    struct stat st;
    if (stat(argvv[4], &st))
    {
        perror("Bad stat\n");
        exit(-1);
    }

    int file_size = st.st_size;

    // First free link and it's corresponding block
    unsigned short start_link = GetLink(0, &ds);
    unsigned int start_block = LinkToBlock(start_link, ds->fat);

    while (true)
    {
        unsigned short free_link = GetLink(0, &ds);
        // Disk is full if link index 0 has value 0.
        if (free_link == 0)
        {
            fprintf("Error: Not enough space for the file. Exit. \n");
            exit(-1);
        }
        // Update Link 0 to reflect the new first free link
        UpdateLink(0, free_link, &ds);
        unsigned int linked_block = LinkToBlock(free_link, ds->fat);

        // Read from file
        int bytes_read = read(fd, buffer, JDISK_SECTOR_SIZE);
        // Error check
        if (bytes_read < 0)
        {
            perror("Error reading from disk.\n");
            close(fd);
            exit(-1);
        }

        // Update file size
        file_size -= bytes_read;

        // Check Case
        // EXACT CASE - File end exists on current "free link", set its reference to 0 to indicate block is full and file is complete
        if (file_size == 0 && bytes_read == JDISK_SECTOR_SIZE)  
        {
            SetLink(free_link, 0);
            jdisk_write(ds->diskptr, linked_block, buffer);
            break;
        }
        // UNEVEN FILE - File end exists on current "free link", set its reference to itself to indicate block is NOT full, but file is complete
        else if (file_size == 0 && bytes_read < JDISK_SECTOR_SIZE)  
        {
            SetLink(free_link, free_link);
            // WHY DO WE DO THIS?
            // If the file is 1 less bytes than max sector size, we use show this special case by setting the last byte to 0xFF.
            // If the file is 2+ bytes smaller than the max sector size, we need to set the last 2 bytes to the size of the file in little endian hex. 
            // This is to prevent us from exporting garbage bytes from the disk later on.
            if (bytes_read == JDISK_SECTOR_SIZE - 1)
            {
                buffer[JDISK_SECTOR_SIZE - 1] = 0xFF;
            }
            else 
            {
                char low_byte = bytes_read & 0xFF; // & 0xFF leaves the least insignficant byte
                char high_byte = (bytes_read >> 8) & 0xFF; // Shifting then &'ing gives the most significant byte
                // Little endian is low sig byte first
                buffer[JDISK_SECTOR_SIZE - 2] = low_byte;
                buffer[JDISK_SECTOR_SIZE - 1] = high_byte;
            }
            SetLink(free_link, free_link);
            jdisk_write(ds->diskptr, linked_block, buffer);
            break;
        }
        else // THERE'S MORE TO READ
        {
            jdisk_write(ds->diskptr, linked_block, buffer);
            free_link = GetLink(0, &ds);
            linked_block = LinkToBlock(free_link, ds->fat);
        }
    }

    // Update Disk
    UpdateDiskFAT(&ds); 
    // Print to user
    printf("File starts at block: %d\n", start_block);
    int num_reads = jdisk_reads(ds->diskptr);
    int num_writes = jdisk_writes(ds->diskptr);
    printf("Reads: %d\nWrites: %d\n", num_reads, num_writes);
    // Clean up
    close(fd);
    exit(0);
} 

void ExportHandler (char** argvv, DiskStats *ds)
{
    // Attach/open disk & error check
    SetDiskValues(&ds, argvv[1]);

    // Check the LBA argument
    int lba;
    // Error checking
    if (sscanf(argvv[3], "%d", &lba) == 0) UsageError((char *) "LBA must be an integer value.\n");
    else if (lba < ds->fat)
    {
        printf("Error in Export: LBA is not for a data sector.\n");
        return -1;
    }
    else if (lba > ds->total)
    {
        printf("Error in Export: LBA too big\n");
        return -1;
    }
    // Starting block is valid -- Open file and read from disk into it
    else 
    {   
        int fd = open(argvv[4], O_WRONLY | O_CREAT, 0666);
        if (fd == -1)
        {
            perror("Error opening input file for reading");
            exit(-1);
        }

       char buffer[JDISK_SECTOR_SIZE];

        unsigned short link_index = BlocktoLink(lba, ds->fat);
        unsigned short link_value = GetLink(link_index, ds);
        int file_size = 0;

        // When link value is 0 or the same as the index, it means we've reached the last block the file is on.
        while(link_value != 0 || link_value != link_index)
        {
            jdisk_read(ds->diskptr, link_index, buffer);
        }
    
        close(fd);
    }
    exit(0);
}

unsigned short GetLink (unsigned short link_index, DiskStats* ds)
{
    int block = link_index/LINKS_PER_PAGE;
    int index_in_block = link_index%LINKS_PER_PAGE;
    // Block has not been read into ds->FATable
    if (ds->FATable[block] == NULL)
    {
        ds->FATable[block] = (unsigned short*)malloc(JDISK_SECTOR_SIZE);
        jdisk_read(ds->diskptr, block, ds->FATable[block]);
    }
    return ds->FATable[block][index_in_block];
}

void UpdateLink (unsigned short link_index, unsigned short new_value, DiskStats* ds)
{
    int block = link_index/LINKS_PER_PAGE;
    int index_in_block = link_index/LINKS_PER_PAGE;
    ds->FATable[block][index_in_block] = new_value;
    ds->updated_blocks[block] = 1;
}

void UpdateDiskFAT (DiskStats* ds)
{
    for (int i = 0; i < ds->fat; i++)
    {
        if(ds->updated_blocks[i])
        {
            jdisk_write(ds->diskptr, i, ds->FATable[i]);
        }
    }
}

//Use for import - Converts actual block number to LBA to return to user
unsigned int LinkToBlock (unsigned short link, int fat)
{
    // Returns actual block 
    return (unsigned int)(link + fat - 1);
}

//Use for export - Convert the LBA from the user to the actual block
unsigned short BlocktoLink (unsigned int block, int fat)
{
    // Returns link index in FAT that corresponds to the block.
    return (unsigned short)(block - fat + 1);
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
