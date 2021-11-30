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
int fileSize = -1;

//check if the file exit, if exit get the pointer to the physical address. return and update the fileSize.
char *check_File(char *map, char *filename, char *pointer)
{
	while(map[0] != DIRECTORY_ENTRY_FREE)
	{
		if(strncmp(map + DIRECTORY_FILENAME_OFFSET, filename, 31) == 0){
			fileSize = map[DIRECTORY_FILE_SIZE_OFFSET] << 24;
			fileSize += map[DIRECTORY_FILE_SIZE_OFFSET+1] << 16;
			fileSize += map[DIRECTORY_FILE_SIZE_OFFSET+2] << 8;
			fileSize += (map[DIRECTORY_FILE_SIZE_OFFSET+3] & 0xFF);
			if(fileSize < 0)
			{	
				fileSize = 0x10000 + fileSize;
			}
			int temp = map[DIRECTORY_START_BLOCK_OFFSET] << 24;
			temp += map[DIRECTORY_START_BLOCK_OFFSET+1] << 16;
			temp += map[DIRECTORY_START_BLOCK_OFFSET+2] << 8;
			//temp += map[DIRECTORY_START_BLOCK_OFFSET+3];
			if(map[DIRECTORY_START_BLOCK_OFFSET+3] >= 0)
			{
				temp += map[DIRECTORY_START_BLOCK_OFFSET+3];
			}
			else
			{
				temp += (map[DIRECTORY_START_BLOCK_OFFSET+3] + 0x100);
			}
			pointer = pointer + temp * superBlockInfo.blockSize;
		}
		map = map + DIRECTORY_ENTRY_SIZE;
	}
	return pointer;
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

int main(int argc, char* argv[])
{
	if(argc != 3)
	{
		fprintf(stderr, "Error: Incorrect usage.\n");
		exit(EXIT_FAILURE);
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
	
	char *file_name = argv[2];
	char *pointer = start;
	map = start;
	readSuperBlockInfoFromImgFile(map);
	
	map = map + superBlockInfo.blockSize * superBlockInfo.rds;
	
	pointer = check_File(map, file_name, pointer);
	
	if(fileSize > 0)
	{
		//printf("%d\n", fileSize);
		FILE *fp = fopen(file_name, "wb");
		fwrite(pointer, sizeof(char), fileSize, fp);
		fclose(fp);
	}
	else
	{
		printf("File not found.\n");
	}
	
	munmap(start, startFile.st_size);
	close(imgfile);
	return 0;
}