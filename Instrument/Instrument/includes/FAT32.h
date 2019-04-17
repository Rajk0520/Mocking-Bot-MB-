/*
 * FAT32.h
 *
 * Created: 30-12-2018 09:28:29
 *  Author: ERTS
 */ 


#ifndef FAT32_H_
#define FAT32_H_


#include "SD.h"


// structure to access Master Boot Record (MBR) for getting info about partitions
struct MBR_info
{
	unsigned char dummy[446];				// ignore
	unsigned char partition_data[64];		// partition records (16 x 4)
	unsigned int  signature;				// 0xAA55
	
};


// structure to access first partition info of the disk
struct Partition_info
{
	unsigned char  status;					// 0x80 - active partition
	unsigned char  head_start;				// starting head
	unsigned short cylsect_start;			// starting cylinder and sector
	unsigned char  type;					// partition type
	unsigned char  headend;					// ending head of the partition
	unsigned short cylsect_end;				// ending cylinder and sector
	unsigned int   first_sector;			// total sectors between MBR and the first sector of the partition
	unsigned int   total_sectors;			// size of this partition in sectors
	
};


// structure to access Boot Sector data
struct BS_info
{
	unsigned char  jump_boot[3];			// default: 0x9000EB
	unsigned char  oem_Name[8];
	unsigned short bytes_per_sector;		// default: 512
	unsigned char  sectors_per_cluster;
	unsigned short reserved_sector_count;
	unsigned char  number_of_FATs;
	unsigned short root_entry_count;
	unsigned short total_sectors_F16;		// must be 0 for FAT32
	unsigned char  media_type;
	unsigned short FATsize_F16;				// must be 0 for FAT32
	unsigned short sectors_per_track;
	unsigned short number_of_heads;
	unsigned long  hidden_sectors;
	unsigned long  total_sectors_F32;
	unsigned long  FATsize_F32;				// count of sectors occupied by one FAT
	unsigned short ext_flags;
	unsigned short fs_version;				// 0x0000 (defines version 0.0)
	unsigned long  root_cluster;			// first cluster of root directory (=2)
	unsigned short fs_info;					// sector number of FS_info structure (=1)
	unsigned short backup_boot_sector;
	unsigned char  reserved[12];
	unsigned char  drive_number;
	unsigned char  reserved1;
	unsigned char  boot_signature;
	unsigned long  volume_id;
	unsigned char  volume_label[11];		// "NO NAME    "
	unsigned char  file_system_type[8];		// "FAT32   "
	unsigned char  boot_data[420];
	unsigned long  boot_end_signature;		// 0xAA55
	
};


// structure to access File Structure Info sector data
struct FS_info
{
	unsigned long lead_signature;			// lead signature used to validate this is FSInfo sector, default: 0x41615252
	unsigned char reserved1[480];
	unsigned long structure_signature;		// another signature, default: 0x61417272
	unsigned long free_cluster_count;		// initial: 0xFFFFFFFF
	unsigned long next_free_cluster;		// initial: 0xFFFFFFFF
	unsigned char reserved2[12];
	unsigned long trail_signature;			// 0xAA550000
	
};


// structure to access Directory Entry in the FAT
struct DIR_info
{
	unsigned char name[11];					// short file name (SFN)
	unsigned char attrib;					// file attributes
	unsigned char nt_reserved;
	unsigned char time_tenth;				// tenths of seconds
	unsigned int  create_time;				// time of file creation
	unsigned int  create_date;				// date of file creation
	unsigned int  last_access_date;			// date when file was last accessed
	unsigned int  first_cluster_high;		// higher word of first cluster number
	unsigned int  write_time;				// time when file was last modified
	unsigned int  write_date;				// date when file was last modified
	unsigned int  first_cluster_low;		// lower word of first cluster number
	unsigned long file_size;				// size of file in bytes
	
};


// attribute definitions for file/directory

#define ATTR_READ_ONLY     0x01
#define ATTR_HIDDEN        0x02
#define ATTR_SYSTEM        0x04
#define ATTR_VOLUME_ID     0x08
#define ATTR_DIRECTORY     0x10
#define ATTR_ARCHIVE       0x20
#define ATTR_LONG_NAME     0x0F

// some constants

#define EMPTY				0x00
#define DELETED				0xE5
#define GET					0
#define SET					1
#define TOTAL_FREE			1
#define NEXT_FREE			2
#define READ				1
#define VERIFY				2
#define GET_FILE			1


// global variables

unsigned long unused_sector, append_file_sector, append_file_location, file_size, append_start_cluster;
unsigned short bytes_per_sector, reserved_sector_count;
unsigned char sectors_per_cluster;
unsigned long first_data_sector, root_cluster, total_clusters;


// member functions

// initializes FAT32 and get the data from the Boot sector of SD card
unsigned char get_boot_sector_data();

// to calculate first sector address of any given cluster number
unsigned long get_first_sector(unsigned long cluster_number);

// to get or set next free cluster or total free clusters in FS_info sector of SD card
unsigned long get_set_free_cluster(unsigned char get_set, unsigned char tot_or_next, unsigned long FS_entry);

// get cluster entry value from FAT to find out the next cluster in the chain
unsigned long get_set_next_cluster(unsigned char get_set, unsigned long cluster_number, unsigned long cluster_entry);

// to convert normal short file name into FAT format
unsigned char convert_file_name(unsigned char *file_name);

// to get DIR/FILE list
struct DIR_info* find_file(unsigned char flag, unsigned char *file_name);

// get the file info (or verify existence of file) from SD card
unsigned char get_file_info(unsigned char flag, unsigned char *file_name);

// read a file from SD card
unsigned char read_file();



#endif /* FAT32_H_ */