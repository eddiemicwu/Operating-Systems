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
int fSize = 0;

void putFile(FILE *disk, char *filename, char *pointer, int startingBlock, int k)
{
	time_t t;
	struct tm *now;
	time(&t);
	now = localtime(&t);
	
	int year = ntohs(now->tm_year + 1900);
	int month = now->tm_mon + 1;
    int day = now->tm_mday;
    int hour = now->tm_hour;
	int min = now->tm_min;
    int sec = now->tm_sec;
	//printf( "%d/%d/%d %d:%d:%d",year, month, day, hour, min, sec);

	fseek(disk, k, SEEK_SET);
	int status = 0x03;
	fwrite(&status, 1, 1, disk);
	fwrite(&startingBlock, 1, 4, disk);
	int numberOfBlock = 0;
	if((fSize % superBlockInfo.blockSize) != 0)
	{
		numberOfBlock =(int)(fSize/superBlockInfo.blockSize) + 1;
	}
	else
	{
		numberOfBlock = fSize/superBlockInfo.blockSize;
	}
	fwrite(&numberOfBlock, 1, 4, disk);
	printf("%d\n", fSize);
    fwrite(&fSize, 1, 4, disk);
	//Create Time
	fwrite(&year, 1, 2, disk);      
	fwrite(&month, 1, 1, disk);      
	fwrite(&day, 1, 1, disk); 
	fwrite(&hour, 1, 1, disk); 
	fwrite(&min, 1, 1, disk);   
	fwrite(&sec, 1, 1, disk);
	//Modify Time
	fwrite(&year, 1, 2, disk);      
    fwrite(&month, 1, 1, disk);      
    fwrite(&day, 1, 1, disk); 
    fwrite(&hour, 1, 1, disk); 
    fwrite(&min, 1, 1, disk);   
    fwrite(&sec, 1, 1, disk);
	//File Name
	unsigned int blank=0x00;
	for(int i = 0; i <= DIRECTORY_MAX_NAME_LENGTH; i++)
	{
		fwrite(&blank, 1, 1, disk);
	}
	fseek(disk, -31, SEEK_CUR);
	fwrite(filename, 1, 31, disk);
	long int unused = 0xFFFFFFFFFFFF;
	fwrite(&unused, 1, 6, disk);
	
	fseek(disk, startingBlock*superBlockInfo.blockSize, SEEK_SET);
	FILE *read = fopen(filename, "rb");
	int tp = 0;
	while(tp != EOF)
	{
		tp =fgetc(read);
		fwrite(&tp, 1, 1, disk);
	}
	//fclose(read);
}

//check if the file exit, if exit get the pointer to the physical address. return and update the fileSize.
char *check_File(FILE *disk, char *map, char *filename, char *pointer)
{
	int start_b = 0;
	int lastSize = 0;
	int fileStart = 0;

	while(map[0] != DIRECTORY_ENTRY_FREE)
	{
		lastSize = map[DIRECTORY_FILE_SIZE_OFFSET] << 24;
		lastSize += map[DIRECTORY_FILE_SIZE_OFFSET+1] << 16;
		lastSize += map[DIRECTORY_FILE_SIZE_OFFSET+2] << 8;
		lastSize += (map[DIRECTORY_FILE_SIZE_OFFSET+3] & 0xFF);
		if(lastSize < 0)
		{	
			lastSize = 0x10000 + lastSize;
		}
		fileStart = map[DIRECTORY_START_BLOCK_OFFSET] << 24;
		fileStart += map[DIRECTORY_START_BLOCK_OFFSET+1] << 16;
		fileStart += map[DIRECTORY_START_BLOCK_OFFSET+2] << 8;
		//temp += map[DIRECTORY_START_BLOCK_OFFSET+3];
		if(map[DIRECTORY_START_BLOCK_OFFSET+3] >= 0)
		{
			fileStart += map[DIRECTORY_START_BLOCK_OFFSET+3];
		}
		else
		{
			fileStart += (map[DIRECTORY_START_BLOCK_OFFSET+3] + 0x100);
		}
		
		if(strncmp(map + DIRECTORY_FILENAME_OFFSET, filename, 31) == 0){
			fileSize = lastSize;
			pointer = pointer + fileStart * superBlockInfo.blockSize;
			return NULL;
		}
		start_b++;
		map = map + DIRECTORY_ENTRY_SIZE;
	}
	
	int k = superBlockInfo.blockSize * superBlockInfo.rds + DIRECTORY_ENTRY_SIZE*start_b;
	if(start_b == 0)
	{
		start_b = superBlockInfo.rds + superBlockInfo.rdb;
	}
	else
	{
		if((lastSize%superBlockInfo.blockSize) != 0)
		{
			start_b = fileStart + (int)(lastSize/superBlockInfo.blockSize) + 1;
		}
		else
		{
			start_b = fileStart + (int)(lastSize/superBlockInfo.blockSize);
		}
	}
	putFile(disk, filename, map, start_b, k);
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
	
	int imgfile = open(argv[1], O_RDWR);
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
	void *start = mmap(NULL, startFile.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, imgfile, 0);
	if (start == MAP_FAILED)
	{
		fprintf(stderr, "Error: failded to map disk.\n");
		close(imgfile);
		exit(EXIT_FAILURE);
	}
	
	char *file_name = argv[2];
	FILE *fp = fopen(file_name, "rb");
	if(fp == NULL)
	{
		printf("File not found.\n");
	}
	else
	{
		fseek(fp, 0, SEEK_END);
		fSize = ftell(fp);
		fclose(fp);
		
		FILE *disk = fopen(argv[1], "r+");
		char *pointer = start;
		map = start;
		readSuperBlockInfoFromImgFile(map);
		map = map + superBlockInfo.blockSize * superBlockInfo.rds;
		
		pointer = check_File(disk, map, file_name, pointer);
		
	}
	
	munmap(start, startFile.st_size);
	close(imgfile);
	return 0;
}