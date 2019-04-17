/*
 * FAT32.c
 *
 * Created: 30-12-2018 09:29:29
 *  Author: ERTS
 */ 


#include "FAT32.h"


// initialize FAT32 and get the data from the Boot sector of SD card
unsigned char get_boot_sector_data()
{
	struct BS_info *boot_sector;
	struct MBR_info *master_boot_rec;
	struct Partition_info *partition;
	
	unsigned long data_sectors;
	unused_sector = 0;
	
	read_single_block(0);
	boot_sector = (struct BS_info *) buffer;
	
	// check if it is boot sector
	if (boot_sector->jump_boot[0] != 0xE9 || boot_sector->jump_boot[0] != 0xEB)
	{
		// if it is not boot sector, it must be MBR
		master_boot_rec = (struct MBR_info *) buffer;
		
		if (master_boot_rec->signature != 0xAA55)
		{
			// if it is not even MBR, then it's not FAT32
			return 0;
		}
		
		partition = (struct Partition_info *) (master_boot_rec->partition_data);
		
		// the unused sectors, hidden to the FAT
		unused_sector = partition->first_sector;
		
		// read the boot sector
		read_single_block(unused_sector);
		
		boot_sector = (struct BS_info *) buffer;
		
		if ((boot_sector->jump_boot[0] != 0xEB) && (boot_sector->jump_boot[0] != 0xE9))
		{
			return 0;
		}
	}
	
	bytes_per_sector = boot_sector->bytes_per_sector;
	sectors_per_cluster = boot_sector->sectors_per_cluster;
	reserved_sector_count = boot_sector->reserved_sector_count;
	
	//lcd_clear();
	// lcd_numeric_value(1, 1, bytes_per_sector, 5);
	// lcd_numeric_value(1, 8, sectors_per_cluster, 5);
	// lcd_numeric_value(2, 1, reserved_sector_count, 5);
	//_delay_ms(500);
	
	root_cluster = boot_sector->root_cluster;
	
	first_data_sector = boot_sector->hidden_sectors + reserved_sector_count
						+ (boot_sector->FATsize_F32 * boot_sector->number_of_FATs);
	
	data_sectors = boot_sector->total_sectors_F32 - reserved_sector_count
					- (boot_sector->number_of_FATs * boot_sector->FATsize_F32);
	
	//lcd_clear();
	//lcd_numeric_value(1, 1, data_sectors - 31090000, 5);		// since number larger than 5 digits cannot be displayed on LCD
	//lcd_numeric_value(2, 1, sectors_per_cluster, 5);
	//_delay_ms(500);
	
	double _tmp = (1.0 / sectors_per_cluster);
	
	total_clusters = (unsigned long)((double)data_sectors * _tmp);
	
	//lcd_clear();
	//lcd_string(1, 1, "clusters: 48");
	//lcd_numeric_value(1, 13, (total_clusters - 480000), 4);		// since number larger than 5 digits cannot be displayed on LCD
	//_delay_ms(500);
	
	// check if FS_info free cluster count is valid
	if (get_set_free_cluster(GET, TOTAL_FREE, 0) < total_clusters)
	{
		//lcd_clear();
		//lcd_string(2, 1, "fs_info correct");
		//_delay_ms(500);
	}
	
	return 1;	
}


// to get or set next free cluster or total free clusters in FS_info sector of SD card
unsigned long get_set_free_cluster(unsigned char get_set, unsigned char tot_or_next, unsigned long FS_entry)
{
	struct FS_info *fs_info_sector;
	
	fs_info_sector = (struct FS_info *) &buffer;
	
	read_single_block(unused_sector + 1);
	
	if ((fs_info_sector->lead_signature != 0x41615252) && (fs_info_sector->structure_signature != 0x61417272) && (fs_info_sector->trail_signature != 0xAA550000))
	{
		return 0;
	}
	
	if (get_set == GET)
	{
		if (tot_or_next == TOTAL_FREE)
		{
			return fs_info_sector->free_cluster_count;
		}
		else	// tot_or_next == NEXT_FREE
		{
			return fs_info_sector->next_free_cluster;
		}
	}
	else	// get_set == SET 			// this is not being used in this Theme
	{
		if (tot_or_next == TOTAL_FREE)
		{
			fs_info_sector->free_cluster_count = FS_entry;
		}
		else	// tot_or_next == NEXT_FREE
		{
			fs_info_sector->next_free_cluster = FS_entry;
		}
	}
	
	return 0xFFFFFFFF;
}


// get cluster entry value from FAT to find out the next cluster in the chain
unsigned long get_set_next_cluster(unsigned char get_set, unsigned long cluster_number, unsigned long cluster_entry)
{
	unsigned int FAT_entry_offset;
	unsigned long *FAT_entry_value, FAT_entry_sector;
	unsigned char retry = 0;
	
	// get sector number of the cluster entry in the FAT
	FAT_entry_sector = unused_sector + reserved_sector_count + ((cluster_number * 4) / bytes_per_sector);
	
	// get the offset address in that sector number
	FAT_entry_offset = (unsigned int) ((cluster_number * 4) % bytes_per_sector);
	
	//lcd_clear();
	//lcd_numeric_value(1, 1, FAT_entry_sector, 5);
	//lcd_numeric_value(2, 1, FAT_entry_offset, 5);
	//_delay_ms(500);
	
	// read the sector into a buffer
	while (retry < 10)
	{
		if (read_single_block(FAT_entry_sector))
		{
			break;
		}		
		retry++;
	}
	
	// get the cluster address from the buffer
	FAT_entry_value = (unsigned long *) &buffer[FAT_entry_offset];
	
	if (get_set == GET)
	{
		return ((*FAT_entry_value) & 0x0FFFFFFF);
	}
	
	// for setting new value in cluster entry in FAT
	*FAT_entry_value = cluster_entry;
	
	return 1;
}


// to convert normal short file name into FAT format
unsigned char convert_file_name(unsigned char *file_name)
{
	unsigned char file_name_FAT[11], j, k;
	
	for (j = 0; j < 12; j++)
	{
		if (file_name[j] == '.')
		{
			break;
		}
	}
	
	if (j > 8)
	{
		//lcd_clear();
		//lcd_string(1, 1, "invalid filename");
		//_delay_ms(500);
	}
	
	// setting file name
	for (k = 0; k < j; k++)
	{
		file_name_FAT[k] = file_name[k];
	}
	
	// filling file name trail with blanks
	for (k = j; k <= 7; k++)
	{
		file_name_FAT[k] = ' ';
	}
	
	j++;
	
	// setting file extension
	for (k = 8; k < 11; k++)
	{
		if (file_name[j] != 0)
		{
			file_name_FAT[k] = file_name[j++];
		}
		// filling extension trail with blanks
		else
		{
			while (k < 11)
			{
				file_name_FAT[k] = ' ';
			}
		}
	}
	
	// converting small letters to caps
	for (j = 0; j < 11; j++)
	{
		if ((file_name_FAT[j] >= 0x61) && file_name_FAT[j] <= 0x7A)
		{
			file_name_FAT[j] = file_name_FAT[j] - 0x20;
		}
	}
	
	for (j = 0; j < 11; j++)
	{
		file_name[j] = file_name_FAT[j];
	}
	
	return 1;
}


// to calculate first sector address of any given cluster number
unsigned long get_first_sector(unsigned long cluster_number)
{
	return (first_data_sector + ((cluster_number - 2) * sectors_per_cluster));
}


// to get DIR/FILE list
struct DIR_info* find_file(unsigned char flag, unsigned char *file_name)
{
	unsigned long cluster, sector, first_sector; //, first_cluster, next_cluster;
	struct DIR_info *dir_info_sector;
	unsigned int i;
	unsigned char j;
	
	cluster = root_cluster;
	
	while (1)
	{
		first_sector = get_first_sector(cluster);
		
		for (sector = 0; sector < sectors_per_cluster; sector++)
		{
			read_single_block(first_sector + sector);
			
			for (i = 0; i < bytes_per_sector; i += 32)
			{
				dir_info_sector = (struct DIR_info *) &buffer[i];
				
				//lcd_clear();
				//lcd_wr_char(1, 1, dir_info_sector->name[0]);
				//lcd_numeric_value(2, 1, dir_info_sector->name[0], 5);
				//_delay_ms(500);
				
				if (dir_info_sector->name[0] == EMPTY)
				{
					if (flag == GET_FILE)
					{
						//lcd_clear();
						//lcd_string(1, 1, "file not exist");
						//_delay_ms(500);
						return 0;
					}
				}
				
				//lcd_clear();
				
				if ((dir_info_sector->name[0] != DELETED) && dir_info_sector->attrib != ATTR_LONG_NAME)
				{
					if (flag == GET_FILE)
					{
						for (j = 0; j < 11; j++)
						{
							//lcd_wr_char(1, j+1, dir_info_sector->name[j]);
							//lcd_numeric_value(2, 1, i, 5);
							//lcd_numeric_value(2, 8, j, 3);
							//_delay_ms(500);
							
							if (dir_info_sector->name[j] != file_name[j])
							{
								break;
							}
							
							if (j == 10)
							{
								/*
								lcd_clear();
								lcd_string(1, 1, file_name);
								lcd_string(2, 1, "file exists");
								_delay_ms(500);
								*/
								if (flag == GET_FILE)
								{
									append_file_sector = first_sector + sector;
									append_file_location = i;
									append_start_cluster = (((unsigned long) dir_info_sector->first_cluster_high) << 16) | dir_info_sector->first_cluster_low;
									file_size = dir_info_sector->file_size;
									
									//lcd_clear();
									//lcd_numeric_value(1, 1, file_size, 5);
									//lcd_numeric_value(2, 1, append_file_sector, 5);
									//_delay_ms(500);
									
									return dir_info_sector;
								}
							}
						}
					}
				}
			}
		}
		
		cluster = get_set_next_cluster(GET, cluster, 0);
		
		if (cluster > 0x0FFFFFF6)
		{
			return 0;
		}
	}
	
	return 0;
}


// get the file info (or verify existence of file) from SD card
unsigned char get_file_info(unsigned char flag, unsigned char *file_name)
{
	struct DIR_info *dir_info_sector;
	
	// convert file name to FAT format
	convert_file_name(file_name);
	
	dir_info_sector = find_file(GET_FILE, file_name);
	
	if (flag == VERIFY)
	{
		return 1;
	}
	
	first_cluster = (((unsigned long) dir_info_sector->first_cluster_high) << 16) | dir_info_sector->first_cluster_low;
	file_size = dir_info_sector->file_size;
	
	curr_pos = 0;
	
	return 1;
}


// read a file from SD card
unsigned char read_file()
{
	unsigned int k;
	
	cluster_start_sector = get_first_sector(first_cluster);
	/*
	lcd_clear();
	lcd_numeric_value(1, 1, cluster_start_sector, 5);
	lcd_numeric_value(2, 1, file_size, 5);
	_delay_ms(500);*/
	
	block_of_cluster = ((curr_pos >> 9) & (sectors_per_cluster - 1));
	
	read_single_block(cluster_start_sector + block_of_cluster);
	
	curr_pos = curr_pos + bytes_per_sector;
	
	// you may have to get the next cluster if the file size
	// is greater than (bytes_per_sector * sectors_per_cluster)
	//cluster_number = get_set_next_cluster(GET, cluster_number, 0);
	
	return 1;
}

	



