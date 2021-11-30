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

void displayInfo()
{
	printf("Super block information:\n");
	printf("Block size: %d\n", superBlockInfo.blockSize);
	printf("Block count: %d\n", superBlockInfo.blockCount);
	printf("FAT starts: %d\n", superBlockInfo.FATs);
	printf("FAT blocks: %d\n", superBlockInfo.FATb);
	printf("Root directory start: %d\n", superBlockInfo.rds);
	printf("Root directory blocks: %d\n", superBlockInfo.rdb);
	printf("\n");
	printf("FAT information:\n");
	printf("Free Blocks: %d\n", numFreeBlocks);
	printf("Reserved Blocks: %d\n", numReservedBlocks);
	printf("Allocated Blocks: %d\n", numAllocatedBlocks);
}

void readSuperBlockInfoFromImgFile(char *point)
{
	int num = 0;
	//block size offset 8 with size 2 bytes trans to Big Endian
	superBlockInfo.blockSize = htons(point[BLOCKSIZE_OFFSET]);
	//block count offset 10 with size 4 bytes trans to Big Endian
	num = point[BLOCKCOUNT_OFFSET] << 24;
	num += point[BLOCKCOUNT_OFFSET+1] << 16;
	num += point[BLOCKCOUNT_OFFSET+2] << 8;
	superBlockInfo.blockCount = num + point[BLOCKCOUNT_OFFSET+3];
	//FATSTART offset 14 with size 4 bytes trans to Big Endian
	num = point[FATSTART_OFFSET] << 24;
	num += point[FATSTART_OFFSET+1] << 16;
	num += point[FATSTART_OFFSET+2] << 8;
	superBlockInfo.FATs = num + point[FATSTART_OFFSET+3];
	//FATBLOCKS offset 18 with size 4 bytes trans to Big Endian
	num = point[FATBLOCKS_OFFSET] << 24;
	num += point[FATBLOCKS_OFFSET+1] << 16;
	num += point[FATBLOCKS_OFFSET+2] << 8;
	superBlockInfo.FATb = num + point[FATBLOCKS_OFFSET+3];
	//ROOTDIRSTART offset 22 with size 4 bytes trans to Big Endian
	num = point[ROOTDIRSTART_OFFSET] << 24;
	num += point[ROOTDIRSTART_OFFSET+1] << 16;
	num += point[ROOTDIRSTART_OFFSET+2] << 8;
	superBlockInfo.rds = num + point[ROOTDIRSTART_OFFSET+3];
	//ROOTDIRBLOCKS offset 26 with size 4 bytes trans to Big Endian
	num = point[ROOTDIRBLOCKS_OFFSET] << 24;
	num += point[ROOTDIRBLOCKS_OFFSET+1] << 16;
	num += point[ROOTDIRBLOCKS_OFFSET+2] << 8;
	superBlockInfo.rdb = num + point[ROOTDIRBLOCKS_OFFSET+3];
}

void analyzeFAT(char *point)
{
	int FATEntryValue = 0;
	int FATEntryAddress = superBlockInfo.FATs * superBlockInfo.blockSize;
	
	while(FATEntryValue < superBlockInfo.blockCount)
	{
		int num = 0;
		num = point[FATEntryAddress] << 24;
		num += point[FATEntryAddress+1] << 16;
		num += point[FATEntryAddress+2] << 8;
		num += point[FATEntryAddress+3];
		if(num == 0)
		{
			numFreeBlocks++;
			FATEntryValue++;
		}
		else if(num == 1)
		{
			numReservedBlocks++;
			FATEntryValue++;
		}
		else
		{
			numAllocatedBlocks++;
			FATEntryValue++;
		}
		FATEntryAddress += FAT_ENTRY_SIZE;
	}
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
	analyzeFAT(map);
	displayInfo();
	
	munmap(start, startFile.st_size);
	close(imgfile);
	return 0;
}