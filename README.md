    Author: Brenna Bentley
    Date: 3/08/2024
    Date of Last Edit: 3/12/2024

    Description: This program is emmulating im- and exporting to/from disks using .jdisk files 
                 instead of actual disks. The main focus is the manipulation of the File Allocation
                 Table, as the read and write operations have already been written by Dr. James Plank 
                 in jdisk.c. 
    For more information look at the html pages in this directory written by Dr. Plank.


    Sources: 
        - I saw Dr. Plank's Usage fx in jdisk_test and decided to implement it as well
        - High/Low Byte: https://stackoverflow.com/questions/6090561/how-to-use-high-and-low-bytes
        - Anika Roskowski and I worked through logic together on paper. Largely on how to minimize read/writes, 
            handling EOF stuff, and updating FAT in import.
        - Anika also spent like 4 hours debugging the small problems with me. Like, using / instead of %. 
            Or referencing link when I should've referenced block. Syntax things that severely broke my code.
        - Note to self: Use unsigned char on for binary fill sign. 
        - Visited Jacob (GTA) in office hours for walk throuhg of write-up, high level logic, and general advice 

    Please ignore all the commented out fprintfs... I was doing a lot of error checking.