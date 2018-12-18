
#include <stdio.h> //Printf, FILE

#include <errno.h> //Error numbers for file handling

#include <string.h> //String handling

#include <stdlib.h> //Malloc

#include <math.h>

char Bitmap_Header[] = 
	{ 
		
		0x42, 0x4D, //offset 0
		0x00, 0x00, 0x00, 0x00, //offset 0x02 - Sizze of bmp file
		0x00, 0x00,0x00,0x00,
		0x36,0x00,0x00,0x00, 
		0x28,0x00,0x00,0x00, 
		
		0x00,0x00,0x00,0x00, //offset 0x12 - size of width
		0x00,0x00,0x00,0x00, //offset 0x16 - size of height
		0x01, 0x00, 
		0x18, 0x00,
		0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00, //offset 0x22 - image size
		0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00
	};
	
// Sets a 4-byte value at a given offset in the header
void setValue(char* head, int value, int offset)
{
	*(int*)(head + offset) = value;
}

void setHeader(int width, int height)
{
	setValue(Bitmap_Header, width * height * 3 + sizeof(Bitmap_Header), 2); //Write bmp size (image size + size of header) to offset 2
	setValue(Bitmap_Header, width, 18); //Write bmp width to offset 18
	setValue(Bitmap_Header, height, 22); //Write bmp height to offset 22
	setValue(Bitmap_Header, width * height * 3, 34); //Write image size (width * height * 3) to offset 34
}

void printHead()
{
	for (int i = 0; i < 54; ++i)
		printf("%x ", (int)(unsigned char)Bitmap_Header[i]);
	printf("\n");
}

//Gets the length of the square side of the output bitmap from the file size
int getSide(int file_size)
{
	int n = ceil(0.5 * log(((double)file_size + 1.0) / 3.0) / log(2.0));
	return ldexp(1.0, n);
}

int check_exists(const char* check_exists)
{
	FILE* fp;
	fp = fopen(check_exists, "rb");
	
	if (fp == NULL)
	{
		if (errno == 2)
			return 0;
		return 1; //????????????????????????? if fopen fails, but its not an error 2 (file not foiund) is this the correct outcome?
	}
	
	fclose(fp);
	
	return 1;
}

int fsize(FILE* file)
{
	int filesize;
	fseek(file, 0l, SEEK_END);
	filesize = ftell(file);
	rewind(file);
	return filesize;
}

int convertToBmp(const char* source, const char* destination)
{
	
	//Open input file to read
	FILE* binary;
	
	binary = fopen(source, "rb");
	
	if (binary == NULL)	
	{
		printf("Error in convertToBmp %i - %s\n", errno, strerror(errno));
		return 0;
	}
	
	//Get size of read file
	int filesize = fsize(binary);
	
	//If read file is greater than 12MB, leave (maximum image size will be 2048*2048. Files larger than this should be split up & compressed
	if (filesize > 12 * 1024 * 1024)
	{
		printf("File size is too large (%i bytes) maximum size is 12MB.", filesize);
		return 0;
	}
	
	//Copy read file's contents to memory
	int side_length = getSide(filesize);
	
	char* binary_data = (char*)malloc(side_length * side_length * 3); //Create array to fit entire pixmap
	
	if (binary_data == NULL)
		return 0;
	
	fread(binary_data, sizeof(*binary_data), filesize, binary);
	
	//Close read file
	fclose(binary);
	
	//Create new file to write to (if file exists, prompt user to overwrite)
	if (check_exists(destination))
	{
		//If the file already exists, open to write, then close. this will effectively erase the contents of the current file.
		fclose(fopen(destination, "wb"));
		
	}
	
	FILE* bitmap;
	
	bitmap = fopen(destination, "ab");
	
	//Modify header
	setHeader(side_length, side_length);
	
	//Write header to file
	fwrite(Bitmap_Header, sizeof(*binary_data), sizeof(Bitmap_Header), bitmap);
	
	//Write size of original file to bitmap in first pixel
	fwrite(&filesize, 3, 1, bitmap);
	
	//Write contents of read file to remaining pixels
	fwrite(binary_data, sizeof(*binary_data), side_length * side_length * 3 - 3, binary);
		
	//Close write file
	fclose(bitmap);
	
	return 0;
}

int convertToBinary(const char* source, const char* destination)
{
	FILE* bitmap;
	
	//Open bitmap file
	bitmap = fopen(source, "rb");
	
	if (bitmap == NULL)	
	{
		printf("Error in convertToBinary %i - %s\n", errno, strerror(errno));
		return 0;
	}	
	
	//Load file into memory
	int filesize = fsize(bitmap);
	
	char* data = (char*)malloc(filesize);
	
	fread(data, sizeof(*data), filesize, bitmap);
	
	fclose(bitmap);
	
	//Get starting address of bitmap data (as per the bitmap header standard)
	int address = *(int*)&data[10];
	
	//Go to starting address, and get the number of bytes in original file
	int original_size = (unsigned char)data[address] + (unsigned char)data[address + 1] * 256 + (unsigned char)data[address + 2] * 65536;
	
	//Save data to file
	FILE* binary;
	
	binary = fopen(destination, "wb");
	
	fwrite(&data[address + 3], sizeof(*data), original_size, binary);
	
	fclose(binary);
	
	return 0;
}

int main(int argc, char **argv)
{
	printf("tobmp started\n");
	
	//Validate arguments and see if output file already exists
	if (argc != 2)
	{
		printf("Invalid number of program arguments\n");
		return 0;
	}
	
	//Create new file name by appending the original filename with ".bmp"
	char* new_file = (char*)malloc(strlen(argv[1]) + 4);
	
	if (new_file == NULL)
		return 0;
	
	strcpy(new_file, argv[1]);
	
	strcat(new_file, ".bmp");
	
	printf("New filename: %s\n", new_file);
	
	convertToBmp(".\\files\\file.zip", ".\\files\\file.zip.bmp");
	
	convertToBinary(".\\files\\file.zip.bmp", ".\\files\\out.zip");
	
	return 0;
}

