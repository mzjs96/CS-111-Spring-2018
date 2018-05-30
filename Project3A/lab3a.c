/*

	NAME: MICHAEL ZHOU,QIAN CAO
	EMAIL: mzjs96@gmail.com,talexcao@gmail.com
	ID: 804663317,404657587
	SLIPDAYS:1

*/

#define _POSIX_C_SOURCE 200809L

#include<stdlib.h>
#include<stdio.h>
#include<unistd.h>
#include<stdint.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<string.h>
#include<fcntl.h>
#include<errno.h>
#include<math.h>
#include<time.h>

#include "ext2_fs.h"

#define UNKNOWN		'?'
#define	DIRECTORY 	'd'
#define REGULAR 	'f'
#define SYMBOLIC 	's'

static int filesys_fd = -1;

static struct ext2_super_block* super_block;	//superblock struct
static struct ext2_group_desc* group;
//char* buffer;
static uint32_t* inode_block;


void super_block_summary()		//check
{
	int ret;	
	//		   file descriptor               struct address          1024				
	ret = pread(filesys_fd, super_block, sizeof(struct ext2_super_block), 1024);
	if(ret < 0){
		fprintf(stderr, "super_block_summary pread failed! Error: %s\n", strerror(errno));
		exit(2);
	}

	//super_block = (struct ext2_super_block*) buffer;
	if( super_block->s_magic != EXT2_SUPER_MAGIC ){
		fprintf(stderr, "File system type is not EXT2! Error: %s\n", strerror(errno));
		exit(2);
	}

	uint32_t blocks_count = super_block->s_blocks_count;
	uint32_t inodes_count = super_block->s_inodes_count;
	uint32_t block_size = EXT2_MIN_BLOCK_SIZE << super_block->s_log_block_size;
	uint32_t inode_size = super_block->s_inode_size;
	uint32_t blocks_per_group = super_block->s_blocks_per_group;
	uint32_t inodes_per_group = super_block->s_inodes_per_group;
	uint32_t first_ino = super_block->s_first_ino;
	/*
		SUPERBLOCK
		total number of blocks (decimal)
		total number of i-nodes (decimal)
		block size (in bytes, decimal)
		i-node size (in bytes, decimal)
		blocks per group (decimal)
		i-nodes per group (decimal)
		first non-reserved i-node (decimal)
	*/
	fprintf(stdout, "SUPERBLOCK,%u,%u,%u,%u,%u,%u,%u\n", blocks_count, inodes_count, block_size, inode_size, blocks_per_group, inodes_per_group, first_ino);
}

void group_summary()		//check
{
	//uint32_t blocks_count = super_block->s_blocks_count;
	//uint32_t inodes_count = super_block->s_inodes_count;

	int ret = pread(filesys_fd, group, sizeof(struct ext2_group_desc), 2048);
	if(ret < 0)
	{
		fprintf(stderr, "group_summary pread failed! Error: %s\n", strerror(errno));
		exit(2);
	}

	uint32_t blocks_per_group = super_block->s_blocks_count;
	uint32_t inodes_per_group = super_block->s_inodes_per_group;

	//group = (struct ext2_group_desc *) buffer;
	uint32_t free_blocks_count = group->bg_free_blocks_count;
	uint32_t free_inodes_count = group->bg_free_inodes_count;
	uint32_t block_bitmap = group->bg_block_bitmap;
	uint32_t inode_bitmap = group->bg_inode_bitmap;
	uint32_t inode_table = group->bg_inode_table;

	fprintf(stdout, "GROUP,%u,%u,%u,%u,%u,%u,%u,%u\n",0,blocks_per_group,inodes_per_group,free_blocks_count,free_inodes_count,block_bitmap,inode_bitmap,inode_table);
	/*
		GROUP
		group number (decimal, starting from zero)
		total number of blocks in this group (decimal)
		total number of i-nodes in this group (decimal)
		number of free blocks (decimal)
		number of free i-nodes (decimal)
		block number of free block bitmap for this group (decimal)
		block number of free i-node bitmap for this group (decimal)
		block number of first block of i-nodes in this group (decimal)
	*/
}

void free_block_summary()	//check
{
	uint32_t block_size = EXT2_MIN_BLOCK_SIZE << super_block->s_log_block_size;
	char byte;
	unsigned int free_block_number;
	int ret;

	for (unsigned int i = 0; i < block_size; i++)
	{	
		//read the each byte from the block bitmap
		ret = pread(filesys_fd, &byte, 1, (block_size * group->bg_block_bitmap) + i);
		if(ret < 0)
		{
			fprintf(stderr, "block_bitmap_summary pread failed! Error: %s\n", strerror(errno));
			exit(2);
		}

		//uint8_t mask = 1;
		for (int j = 0; j < 8; j++)
		{	
			//using mask to identify whether this is a free block
			if((byte & (1 << j)) == 0)
			{
				//get the offset number of the free block
				free_block_number = i * 8 + j + 1;
				fprintf(stdout, "BFREE,%d\n", free_block_number);
			}
		//mask = mask << 1;
		}
	}
}

void free_inode_summary()
{
	uint32_t block_size = EXT2_MIN_BLOCK_SIZE << super_block->s_log_block_size;
	char byte;
	unsigned int free_inode_number;
	int ret;

	for(unsigned int i = 0; i < block_size; i++)
	{
		ret = pread(filesys_fd, &byte, 1, group->bg_inode_bitmap * block_size + i);
		if(ret < 0)
		{
			fprintf(stderr, "Fail to read: %s\n", strerror(errno));
			exit(2);
		}

		for(int j = 0; j < 8; j++) 
		{
			if((byte & (1 << j)) == 0) 
			{
				free_inode_number = i * 8 + j + 1;
				fprintf(stdout, "IFREE,%d\n", free_inode_number);
			}
		}
	}
}

void indirect_summary(int p_inode_number, int block_number, int block_offset, int level)
{
	uint32_t block_size = EXT2_MIN_BLOCK_SIZE << super_block->s_log_block_size;
	uint32_t indirect_num = block_size/4;

	uint32_t buf[indirect_num];

	//buf = (uint32_t *) malloc (indirect_num * sizeof(uint32_t));
	memset(buf, 0, indirect_num * sizeof(uint32_t));
	
	int ret;
	ret = pread(filesys_fd, buf, block_size, block_number* block_size);
	if (ret < 0)
	{
		fprintf(stderr, "indirect_summary pread failed! Error: %s\n", strerror(errno));
		exit(2);
	}

	for (unsigned int i = 0; i < indirect_num; i++)
	{
		int block = buf[i];
		//if the block is valid, output the corresponding csv
		if( block != 0 )
		{
			/*
			 * Indirect blocks 
			 * 1. I-node number of the owning file (decimal)
			 * 2. (decimal) level of indirection for the block being scanned ... 1 for single indirect, 2 for double indirect, 3 for triple
			 * 3. logical block offset (decimal) represented by the referenced block. 
			 * 4. block number of the (1, 2, 3) indirect block being scanned (decimal)
			 * 5. block number of the referenced block (decimal)
			 */
			fprintf(stdout, "INDIRECT,%d,%d,%d,%d,%d\n", p_inode_number, level, block_offset, block_number, block);
			if (level == 1)
			{
				block_offset++;
			}
			else if(level == 2 || level == 3)
			{	
				indirect_summary(p_inode_number, block, block_offset, level-1);
			}
		}
		else
		{
			if(level == 1)
				block_offset++;
			else if(level == 2)
				block_offset += 256;
			else if(level == 3)
				block_offset += 65536;
		}
	}
}

void directory_summary(int p_inode_number)
{
	struct ext2_dir_entry *dir;
	dir = (struct ext2_dir_entry *) malloc (sizeof(struct ext2_dir_entry));
	uint32_t block_size = EXT2_MIN_BLOCK_SIZE << super_block->s_log_block_size;
	int ret;
	for (int i = 0; i < EXT2_N_BLOCKS; ++i)
	{
		if (inode_block[i] == 0)
			break;
		unsigned int logical_offset = 0;
		int start_offset = inode_block[i] * block_size;
		while(logical_offset < block_size)
		{
			ret = pread(filesys_fd, dir, sizeof(struct ext2_dir_entry), start_offset + logical_offset);
			if (ret < 0)
			{
				fprintf(stderr, "directory_summary pread failed! Error: %s\n", strerror(errno));
				exit(2);
			}

			//to check if the directory inode is valid
			if( dir->inode != 0)
				fprintf(stdout, "DIRENT,%d,%d,%d,%d,%d,'%s'\n", p_inode_number, logical_offset, dir->inode, dir->rec_len, dir->name_len, dir->name);
			
			logical_offset += dir->rec_len;
		}
		/*
		DIRENT
			parent inode number (decimal) ... the I-node number of the directory that contains this entry
			logical byte offset (decimal) of this entry within the directory
			inode number of the referenced file (decimal)
			entry length (decimal)
			name length (decimal)
			name (string, surrounded by single-quotes). Don't worry about escaping, we promise there will be no single-quotes or commas in any of the file names.
		*/
	}
}

void inode_summary()
{
	struct ext2_inode* inode;
	inode = (struct ext2_inode*) malloc(sizeof(struct ext2_inode));
	//allocate memory for the current inode 

	uint32_t inode_size = super_block->s_inode_size;
	//uint32_t inodes_count = super_block->s_inodes_count;
	uint32_t block_size = EXT2_MIN_BLOCK_SIZE << super_block->s_log_block_size;
	uint32_t inodes_per_group = super_block->s_inodes_per_group;
	uint32_t inode_bitmap = group->bg_inode_bitmap;
	uint32_t inode_table = group->bg_inode_table;

	char file_type;
	unsigned int inode_number;
	int ret;
	int flag=0;
	char byte;

	//uint32_t inode_bitmap = group->bg_inode_bitmap;
	for (unsigned int i = 0; i < block_size; i++)
	{
		ret = pread(filesys_fd, &byte, 1, inode_bitmap * block_size + i);
		if(ret < 0)
		{
			fprintf(stderr, "inode summary byte pread failed! Error: %s\n", strerror(errno));
			exit(2);
		}

		for (int j = 0; j < 8; j++)
		{
			inode_number = i*8 + j+1;
			if (inode_number > inodes_per_group)
			{
				flag = 1;
				break;
			}

			if ((byte & (1<<j)))	//if there is exists an inode in the bitmap
			{
				ret = pread(filesys_fd, inode, inode_size, inode_table * block_size + inode_size*(inode_number - 1));
				if (ret < 0)
				{
					fprintf(stderr, "inode summary inode pread failed! Error: %s\n", strerror(errno));
					exit(2);
				}

				//check if it is valid;
				if ((inode->i_mode == 0) || (inode->i_links_count == 0))
					continue;

				uint16_t owner = inode->i_uid;
				uint16_t group = inode->i_gid;
				uint16_t link_count = inode->i_links_count;
				uint32_t size = inode->i_size;
				uint32_t blocks = inode->i_blocks;

				file_type = UNKNOWN;
				if (S_ISDIR(inode->i_mode)) 
				{
					file_type = DIRECTORY;
				} else if (S_ISREG(inode->i_mode)) 
				{
					file_type = REGULAR;
				} else if (S_ISLNK(inode->i_mode)) 
				{
					file_type = SYMBOLIC;
				}

				time_t time;

				char created_tm[18];	//created time
				time = inode->i_ctime;
				struct tm* ctime = gmtime(&time);
				strftime(created_tm, 18, "%D %H:%M:%S", ctime);

				char modified_tm[18];	//modified time
				time = inode->i_mtime;
				struct tm* mtime = gmtime(&time);
				strftime(modified_tm, 18, "%D %H:%M:%S", mtime);

				char accessed_tm[18];	//accessed time
				time = inode->i_atime;
				struct tm* atime = gmtime(&time);
				strftime(accessed_tm, 18, "%D %H:%M:%S", atime);

				/*
				INODE:
					inode number (decimal)
					file type ('f' for file, 'd' for directory, 's' for symbolic link, '?" for anything else)
					mode (low order 12-bits, octal ... suggested format "%o")
					owner (decimal)
					group (decimal)
					link count (decimal)
					time of last I-node change (mm/dd/yy hh:mm:ss, GMT)
					modification time (mm/dd/yy hh:mm:ss, GMT)
					time of last access (mm/dd/yy hh:mm:ss, GMT)
					file size (decimal)
					number of (512 byte) blocks of disk space (decimal) taken up by this file
				*/

				fprintf(stdout, "INODE,%u,%c,%o,%u,%u,%u,%s,%s,%s,%d,%d", inode_number, file_type, inode->i_mode & 0xFFF, owner, group, link_count, created_tm, modified_tm, accessed_tm, size, blocks);
				//the next fifteen fields are block addresses

				if( file_type == SYMBOLIC && inode->i_size < 60) 	//don't quite understand this part (actually this is on PIAZZA and it was technically incorrect)
				{
					fprintf(stdout, ",%d", inode->i_block[0]);
				}
				else
				{
					for (int k = 0; k < 15; k++) 
					{
						fprintf(stdout, ",%d", inode->i_block[k]);
					}
				}
				fprintf(stdout, "\n");

				inode_block = inode->i_block;	//pointer to the 15 blocks
				if(file_type == DIRECTORY)
				{	
					directory_summary(inode_number);
					indirect_summary(inode_number, inode_block[12], 12, 1);
					indirect_summary(inode_number, inode_block[13], 268, 2);
					indirect_summary(inode_number, inode_block[14], 65804, 3);
					//void indirect_summary(int p_inode_number, int block_number, int block_offset, int level)
				}
				else if(file_type == REGULAR)
				{
					indirect_summary(inode_number, inode_block[12], 12, 1);
					indirect_summary(inode_number, inode_block[13], 268, 2);
					indirect_summary(inode_number, inode_block[14], 65804, 3);
				}
			}
		}
		if (flag == 1)
			break;
	}
}

int main(int argc, char* argv[])
{
	if(argc!= 2)
	{
		fprintf(stderr, "Error number of arguments! %s\n", strerror(errno));
		exit(1);
	}
	const char* fs_img = argv[1];

	filesys_fd = open(fs_img, O_RDONLY);

	if (filesys_fd < 0)
	{
		fprintf(stderr, "File system file descriptor open failed! %s\n", strerror(errno));
		exit(2);
	}


	//buffer = (char*) malloc(1024);
	super_block = (struct ext2_super_block*) malloc(sizeof(struct ext2_super_block));
	group = (struct ext2_group_desc*) malloc(sizeof(struct ext2_group_desc));
	inode_block = (uint32_t*) malloc(sizeof(uint32_t)*EXT2_N_BLOCKS);

	super_block_summary();
	group_summary();
	free_block_summary();
	free_inode_summary();
	inode_summary();

	exit(0);

}
