/*
CSC360 Assignment3
Name: Wenbo Wu
V#: V00928620
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include "Constants.h"

struct __attribute__((__packed__)) SuperBlockInfo
{
	uint8_t fileSystemIdentifier[8];
	uint16_t blockSize;
	uint32_t blockCount;
	uint32_t FATs;
	uint32_t FATb;
	uint32_t rds;
	uint32_t rdb;
};

struct SuperBlockInfo superBlockInfo;
int numFreeBlocks = 0;
int numAllocatedBlocks = 0;
int numReservedBlocks = 0;

void readRootDirectory(char *pointer)
{
	//check the status if is directory 0x10.
	int tmp = pointer[DIRECTORY_STATUS_OFFSET];
	//printf("%x\n", tmp);
	if( (tmp & 0x10) == 0x10)
	{
		printf("D ");
	}
	else
	{
		printf("F ");
	}
	
	int fileSize = pointer[DIRECTORY_FILE_SIZE_OFFSET] << 24;
	fileSize += pointer[DIRECTORY_FILE_SIZE_OFFSET+1] << 16;
	fileSize += pointer[DIRECTORY_FILE_SIZE_OFFSET+2] << 8;
	fileSize += (pointer[DIRECTORY_FILE_SIZE_OFFSET+3] & 0xFF);
	
	if(fileSize > 0)
	{
		printf("    %6d", fileSize);
	}
	else
	{
		printf("    %6d", 0x10000 + fileSize);
	}
	//print 31 char as filename
	printf("%31s", pointer + DIRECTORY_FILENAME_OFFSET); 
	
	tmp = (pointer[DIRECTORY_MODIFY_OFFSET] << 8) + (pointer[DIRECTORY_MODIFY_OFFSET+1]);
	//printf("\n %d \n", ntohs(pointer[DIRECTORY_MODIFY_OFFSET]));
	if(pointer[DIRECTORY_MODIFY_OFFSET+1] < 0)
	{
		printf(" %d", tmp + 0x100);
	}
	else
	{
		printf(" %d", tmp);
	}
	// printf("\n%02d\n", pointer[DIRECTORY_MODIFY_OFFSET+2]);
	// printf("\n%02d\n", pointer[DIRECTORY_MODIFY_OFFSET+3]);
	// printf("\n%02d\n", pointer[DIRECTORY_MODIFY_OFFSET+4]);
	// printf("\n%02d\n", pointer[DIRECTORY_MODIFY_OFFSET+5]);
	// printf("\n%02d\n", pointer[DIRECTORY_MODIFY_OFFSET+6]);
	printf("/%02d/%02d %02d:%02d:%02d\n", pointer[DIRECTORY_MODIFY_OFFSET+2], 
			pointer[DIRECTORY_MODIFY_OFFSET+3], pointer[DIRECTORY_MODIFY_OFFSET+4], 
			pointer[DIRECTORY_MODIFY_OFFSET+5], pointer[DIRECTORY_MODIFY_OFFSET+6]);
}

void readSuperBlockInfoFromImgFile(char *pointer)
{
	int temp = 0;
	//block size offset 8 with size 2 bytes trans to Big Endian
	superBlockInfo.blockSize = htons(pointer[BLOCKSIZE_OFFSET]);
	//block count offset 10 with size 4 bytes trans to Big Endian
	temp = pointer[BLOCKCOUNT_OFFSET] << 24;
	temp += pointer[BLOCKCOUNT_OFFSET+1] << 16;
	temp += pointer[BLOCKCOUNT_OFFSET+2] << 8;
	superBlockInfo.blockCount = temp + pointer[BLOCKCOUNT_OFFSET+3];
	//FATSTART offset 14 with size 4 bytes trans to Big Endian
	temp = pointer[FATSTART_OFFSET] << 24;
	temp += pointer[FATSTART_OFFSET+1] << 16;
	temp += pointer[FATSTART_OFFSET+2] << 8;
	superBlockInfo.FATs = temp + pointer[FATSTART_OFFSET+3];
	//FATBLOCKS offset 18 with size 4 bytes trans to Big Endian
	temp = pointer[FATBLOCKS_OFFSET] << 24;
	temp += pointer[FATBLOCKS_OFFSET+1] << 16;
	temp += pointer[FATBLOCKS_OFFSET+2] << 8;
	superBlockInfo.FATb = temp + pointer[FATBLOCKS_OFFSET+3];
	//ROOTDIRSTART offset 22 with size 4 bytes trans to Big Endian
	temp = pointer[ROOTDIRSTART_OFFSET] << 24;
	temp += pointer[ROOTDIRSTART_OFFSET+1] << 16;
	temp += pointer[ROOTDIRSTART_OFFSET+2] << 8;
	superBlockInfo.rds = temp + pointer[ROOTDIRSTART_OFFSET+3];
	//ROOTDIRBLOCKS offset 26 with size 4 bytes trans to Big Endian
	temp = pointer[ROOTDIRBLOCKS_OFFSET] << 24;
	temp += pointer[ROOTDIRBLOCKS_OFFSET+1] << 16;
	temp += pointer[ROOTDIRBLOCKS_OFFSET+2] << 8;
	superBlockInfo.rdb = temp + pointer[ROOTDIRBLOCKS_OFFSET+3];
}

int main(int argc, char* argv[])
{
	if(argc != 2)
	{
		fprintf(stderr, "Error input\n");
		return -1;
	}
	int imgfile = open(argv[1], O_RDONLY);
	if(imgfile < 0)
	{
		perror("Error opening file for reading");
		exit(EXIT_FAILURE);
	}
	
	char *map;
	struct stat startFile;
	if(fstat(imgfile, &startFile) < 0)
	{
		perror("STAT ERROR");
		exit(EXIT_FAILURE);
	}
	void *start = mmap(NULL, startFile.st_size, PROT_READ, MAP_SHARED, imgfile, 0);
	if (start == MAP_FAILED)
		return -1;
	
	map = start;
	readSuperBlockInfoFromImgFile(map);
	map = map + superBlockInfo.blockSize * superBlockInfo.rds;
	while(map[0] != DIRECTORY_ENTRY_FREE)
	{
		readRootDirectory(map);
		map = map + DIRECTORY_ENTRY_SIZE;
	}

	munmap(start, startFile.st_size);
	close(imgfile);
	return 0;
}