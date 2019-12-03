
#include <stdio.h> //Printf, FILE

#include <errno.h> //Error numbers for file handling

#include <string.h> //String handling

#include <stdlib.h> //Malloc

#include <math.h> //ceil, sqrt

#include <stdlib.h> //system

// Hint: a file size of 16765487 bytes will convert to an image under 16MB, perfect for google photo storage!

#define BYTES_PER_PIXEL sizeof(int) // The number of bytes used to represent the length of padding

const char* TRUNC_EXE_PATH = "C:\\trunc.exe ";
const char* RENAME_EXEC_PATH = "move ";

//Array of bytes containing bitmap header
char Bitmap_Header[] = 
{ 
	
	0x42, 0x4D, 
	0x9A, 0x00, 0x00, 0x00, //Size of BMP file
	0x00, 0x00, 0x00, 0x00, 
	0x7A, 0x00, 0x00, 0x00, //Offset where pixelarray begins
	//End of BMP header, beginning of DIB header
	0x6C, 0x00, 0x00, 0x00, //Number of bytes in DIB header
	0x04, 0x00, 0x00, 0x00, //Width
	0x02, 0x00, 0x00, 0x00, //Height
	0x01, 0x00, 0x20, 0x00, 0x03, 0x00, 0x00, 0x00, 
	0x20, 0x00, 0x00, 0x00, //Size of bitmap data
	0x13, 0x0B, 0x00, 0x00, 0x13, 0x0B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0xFF, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x20, 0x6E, 0x69, 0x57, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00

};

//Head of file is used to store beginning of original file (when converting to bmp) or bitmap header and first pixel (when converting back to binary)
char File_Head[sizeof(Bitmap_Header) + BYTES_PER_PIXEL];

//Enumerates verbosity and whether or not the user is to be prompted for input
enum Prompt { Loud, Quiet };

//Structure containing the options used 
struct Options
{
	enum Prompt prompt;
} _args = { Loud };

//Resize file at path to size bytes
void resizefile(const char* path, int size)
{
	char ssize[20];
	char string[100] = "";
	sprintf(ssize, " %d", size);
	
	strcat(strcat(strcat(string, TRUNC_EXE_PATH), path), ssize);
	
	system(string);
	
	printf("Call: %s\n", string);
}

void appendfilename(const char* path)
{
	char command[150];
	
	strcpy(command, RENAME_EXEC_PATH);
	
	strcat(strcat(strcat(strcat(command, path), " "), path), ".bmp");
	
	printf("Rename: %s\n", command);
	
	system(command);
}

void truncatefilename(const char* path)
{
	char command[150];
	char newpath[150];
	
	strcpy(command, RENAME_EXEC_PATH);
	strcpy(newpath, path);
	
	newpath[strlen(path) - 4] = '\0'; //Truncate a string the fun way!
	
	strcat(strcat(strcat(command, path), " "), newpath);
	
	printf("Rename: %s\n", newpath);
	
	system(command);
	
}

// Sets a 4-byte value at a given offset in the header
void setValue(char* head, int value, int offset)
{
	*(int*)(head + offset) = value;
}

//Sets the attributes in the header
void setHeader(int width, int height)
{
	setValue(Bitmap_Header, width * height * 3 + sizeof(Bitmap_Header), 2); //Write bmp size (image size + size of header) to offset 2
	setValue(Bitmap_Header, width, 18); //Write bmp width to offset 18
	setValue(Bitmap_Header, height, 22); //Write bmp height to offset 22
	setValue(Bitmap_Header, width * height * 3, 34); //Write image size, in bytes (width * height * 3) to offset 34
}

//Gets the length of the square side of the output bitmap from the file size
int getWidth(int file_size)
{
	return ceil(sqrt(file_size / (float)BYTES_PER_PIXEL + 1.0));
}

//Gets the length of the square side of the output bitmap from the file size
int getHeight(int file_size, int width)
{
	return ceil(((float)file_size + 4.0) / (width * 4));
}

//Get the size of the file
int fsize(FILE* file)
{
	int filesize;
	fseek(file, 0l, SEEK_END);
	filesize = ftell(file);
	rewind(file);
	return filesize;
}

//Convert source to bitmap file in destination
int convertToBmp(const char* source)
{
	printf("Converting from binary to bmp...\n");
	
	//Open input file to read
	FILE* binary;
	
	binary = fopen(source, "rb+");
	
	if (binary == NULL)	
	{
		printf("Error in convertToBmp %i - %s\n", errno, strerror(errno));
		return -1;
	}
	
	//Get size of file to convert
	int filesize = fsize(binary);
	
	//Calculate the width and height of the pixmap 
	int side_width = getWidth(filesize);
	
	int side_height = getHeight(filesize, side_width);
	
	printf("Pixmap size: %i x %i\n", side_width, side_height);
	
	//Calculate padding required for bitmap
	long long pixmap_size = (long long)side_width * (long long)side_height * 4;
	
	int pad_size = pixmap_size - filesize - 4;
	
	printf("Padding size: %i\n", pad_size);
	
	printf("File size: %i\n", filesize);
	
	//Test to make sure file size > sizeof(File_Head)
	
	if (filesize < 127)		
	{
		
	}
	
	//Copy first 126 bytes (size of File_Head) from original file into memory
	fread(File_Head, sizeof(File_Head), 1, binary);
	
	//Append the copied bytes to file
	fseek(binary, 0, SEEK_END);
	fwrite(File_Head,  sizeof(File_Head), 1, binary);
	
	printf("Head file: %i\n", sizeof(File_Head));
	
	//Modify BMP header
	setHeader(side_width, side_height);
	
	//Go to beginning of stream
	fseek(binary, 0, SEEK_SET);
	
	//Write BMP header to file
	fwrite(Bitmap_Header, sizeof(Bitmap_Header), 1, binary);
	
	//Write size of padding to bitmap in first pixel
	fwrite(&pad_size, 1, BYTES_PER_PIXEL, binary);
		
	//Close write file
	fclose(binary);
	
	//Resize to add padding
	resizefile(source, pixmap_size + sizeof(Bitmap_Header));
	
	//Add .bmp to filename
	appendfilename(source);
	
	return 0;
}

//Convert source bitmap back to binary file in destination
int convertToBinary(const char* source)
{
	printf("Convert bmp to binary file\n");
	
	FILE* bitmap;
	
	//Open bitmap file
	bitmap = fopen(source, "rb+");
	
	if (bitmap == NULL)	
	{
		printf("Error in convertToBinary %i - %s\n", errno, strerror(errno));
		return -1;
	}	
	
	//Load bitmap header into memory
	int filesize = fsize(bitmap);
	
	fread(File_Head, sizeof(File_Head), 1, bitmap);
	
	//Perform some tests to make sure file is valid bitmap
		// Check padding isnt larger than pixmap
		// Check first few characters are BM
	
	//Get starting address of bitmap data (as per the bitmap header standard)
	int address = *(int*)&File_Head[10];
	
	//Go to the first pixel, get the size of padding
	int padding_size = *(int*)&File_Head[address];
	
	printf("Padding size: %i\n", padding_size);
	
	//Get the location of the head
	int offset = filesize - padding_size - sizeof(File_Head);
	
	printf("Offset: %i\n", offset);
	
	//Go to the head
	fseek(bitmap, offset, SEEK_SET);
	
	//Read the head
	fread(File_Head, sizeof(File_Head), 1, bitmap);
	
	//Go to beginning
	fseek(bitmap, 0, SEEK_SET);
	
	//Write the head back to the beginning
	fwrite(File_Head, sizeof(File_Head), 1, bitmap);
	
	//Close file
	fclose(bitmap);
	
	//Truncate file to remove padding
	resizefile(source, offset);
	
	//Rename file
	truncatefilename(source);
	
	return 0;
}

int isOption(const char* arg)
{
	return (int)(arg[0] == '-');
}

int  setOption(const char* option)
{
	if (strcmp(option, "-q") == 0)
	{
		_args.prompt = Quiet;
	} else if (strcmp(option, "-l") == 0)
	{
		_args.prompt = Loud;
	}
	else
	{
		printf("Invalid command line option %s\n", option);
		return -1;
	}
	return 0;
}

int main(int argc, char **argv)
{	
	char* source;
	
	if (argc == 1) //No command line arguments supplied, error.
	{
		printf("No input file. Program terminated.\n");
		return 0;
	}
	else if (argc == 2) //One argument supplied.
	{
		if (isOption(argv[1]))
		{
			printf("At least one path must be supplied\n");
			return 0;
		}
		
		source = argv[1];
		
	}
	else if (argc >= 3) // two args. Option and path, or path and option
	{		
		if (isOption(argv[2]))
		{
			printf("Options must be listed before path.\n");
			return 0;
		}
		
		setOption(argv[1]);
		source = argv[2];		
	}	
	
	printf("Source     : %s\n", source);
	printf("Prompt     : %s\n", (_args.prompt == Quiet ? "Quiet" : "Loud"));
	
	//Get last 4 characters of source as string
	char* sub = &source[strlen(source) - 4];
	
	if (strcmp(sub, ".bmp") == 0)
	{
		printf("Direction  : Bmp to Bin\n");
		convertToBinary(source);
	}
	else
	{
		printf("Direction  : Bin to Bmp\n");
		convertToBmp(source);
	}
		
	printf("Conversion complete\n");
	
	
	return 0;
}

