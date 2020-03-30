
General Information:

    This test suite is designed to be used with volumes of 1GB in size,
    which allows for a really short processing time.

    It is used on a virtual machine to test the algorithm.

    The following file systems are supported:

        1) FAT
        2) FAT32
        3) exFAT (1)
        4) NTFS
        5) NTFS compressed
           (with compression enabled)
        6) NTFS mixed
           (with 25% of compressed files, which mimics a regular system disk)
        7) UDF v1.02 (2)
        8) UDF v1.50 (2)
        9) UDF v2.00 (2)
       10) UDF v2.01 (2)
       11) UDF v2.50 (2)
       12) UDF v2.50 with duplicated meta-data (2)

    The fragmentation utility from http://www.mydefrag.com/ is used to create
    fragmented files.
    
    For cloning the disks to test different settings on the same data you may use
    HDClone available freely at http://www.miray.de/products/sat.hdclone.html
    
    You can change the number of fragmented files by not formating the drive.
    This way the current files will be fragmented.

        (1) exFAT is only included with Vista and above.
            For Windows XP SP2 and SP3 you need to download and install
            the driver from http://support.microsoft.com/kb/955704/en-us
            Windows XP SP1 and below do not support exFAT.

        (2) UDF is only included with Vista and above.

---

                    !!! CAUTION !!!

    If you select to format the volume make sure to not use a volume,
    where you have valuable data stored else your data is lost.

    It is best to use virtual test volumes without any existing data.

---

Setup:

    1) Prepare separate volumes, 1GB in size with the disk management utility
    
    2) assign a volume letter and any file system, so it can be found by the utility
    
        a) You will be asked for the volume to process, which you select from the menu

        b) If you format the volume you will be asked to select one of the supported file systems

    3) CHKDSK is executed before and after the fragmented files creation process,
       to make sure the volume is consistent.

    4) It is best to always format the volume to always start with a fresh volume,
       this will detect bad clusters.
       In addition do only use volumes of 1GB in size, since quick format is not used, which
       would only clear the file allocation table and would not detect bad clusters.
