
#include <stdio.h> //Printf, FILE, rename

#include <errno.h> //Error numbers for file handling

#include <string.h> //String handling

#include <stdlib.h> //Malloc

#include <math.h> //ceil, sqrt

#include <stdlib.h> //system

#include <stdint.h> //uint32_t, uint16_t

//Platform specific headers, used for file truncation and renaming
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

const int BYTES_PER_PIXEL = 4; // The number of bytes used to represent the length of padding


//Force compiler to use 2 byte packing
#pragma pack(2)
struct BitmapV4Header
{
	//BMP Header
	uint16_t ID; 			//'BM' at beginning
	uint32_t fileSize;		//Total file size
	uint32_t UNUSED1;
	uint32_t offset;		//Starting address of pixmap
	
	//DIB Header
	uint32_t DIBSize;		//Size of DIB header
	uint32_t width;			//Width of pixmap
	uint32_t height;		//height of pixmap
	uint16_t pbnlanes;
	uint16_t bpp;			//Bits per pixel
	uint32_t compression;
	uint32_t pixmapSize;	//Size of pixmap, in bytes
	uint32_t horizontal;
	uint32_t vertical; 
	uint32_t palette;
	uint32_t important;
	uint32_t redMask;
	uint32_t greenMask;
	uint32_t blueMask;
	uint32_t alphaMask;
	uint32_t win;
	uint8_t UNUSED2[36];
	uint32_t redGamma;
	uint32_t greenGamma;
	uint32_t blueGamma;
	
} 
//Template header
Bitmap_Header = { 0x4D42, 0, 0, 0x7A, 0x6C, 0, 0, 1, 32, 3, 0, 4000, 4000, 0, 0, 0xFF0000, 0xFF00, 0xFF, 0xFF000000, 0x57696E20, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0 };

//Header containing information specific to the toBmp conversion
struct BTBHeader
{
	uint32_t paddingSize; 	//Size of padding, in bytes
	uint32_t signature[4];	//4 pixel signature, stamped into every bitmap
} btbHeader = { 0, {0x6FAFEC0D,  0x7EF10C44, 0x68E85B0B, 0x9C0FB9EF } };

//Head of file is used to store beginning of original file (when converting to bmp) or bitmap header and btb header (when converting back to binary)
#pragma pack(2)
struct CompleteHeader
{
	struct BitmapV4Header bmp;
	struct BTBHeader btb;
} completeHeader;

//Platform independent function to resize file at path to size bytes
int resizefile(const char* path, int size)
{
#ifdef _WIN32
	//Get file handle for path
	HANDLE file = CreateFileA(path, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	
	if (file == INVALID_HANDLE_VALUE)
	{
		fprintf(stderr, "Error: Windows API returned error code %li. Visit 'https://docs.microsoft.com/en-us/windows/win32/debug/system-error-codes' for the error code description.\n", GetLastError());
		return 1;
	}
	
	//Set file pointer to new file size
	if (SetFilePointer(file, size, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
	{
		fprintf(stderr, "Error: Windows API returned error code %li. Visit 'https://docs.microsoft.com/en-us/windows/win32/debug/system-error-codes' for the error code description.\n", GetLastError());
		return 1;
	}
	
	//Set end of file to position of file pointer
	if (!SetEndOfFile(file))
	{
		fprintf(stderr, "Error: Windows API returned error code %li. Visit 'https://docs.microsoft.com/en-us/windows/win32/debug/system-error-codes' for the error code description.\n", GetLastError());
		return 1;
	}
	
	//Close file
	if (!CloseHandle(file))
	{
		fprintf(stderr, "Error: Windows API returned error code %li. Visit 'https://docs.microsoft.com/en-us/windows/win32/debug/system-error-codes' for the error code description.\n", GetLastError());
		return 1;
	}

#else
	if (truncate(path, size))
	{
		fprintf(stderr, "Error: Linux call to truncate() failed with error code %i - %s\n", errno, strerror(errno));
		return 1;
	}
	
#endif

	return 0;

}

int appendfilename(const char* path)
{
	char new[150];
	
	strcpy(new, path);
	strcat(new, ".bmp");
	
	if (rename(path, new))
	{
		fprintf(stderr, "Error: Could not append file name. Error %i - %s\n", errno, strerror(errno));
		return 1;
	}
	
	return 0;
	
}

int truncatefilename(const char* path)
{
	char newpath[150];
	
	strcpy(newpath, path);
	
	newpath[strlen(path) - 4] = '\0'; //Truncate a string the fun way!	
	
	if (rename(path, newpath))
	{
		fprintf(stderr, "Error: Could not truncate file name. Error %i - %s\n", errno, strerror(errno));
		return 1;
	}
	
	return 0;
}

//Calculate the width of the pixmap from the file size
int getWidth(int file_size)
{
	return ceil(sqrt((float)(file_size + sizeof(btbHeader)) / BYTES_PER_PIXEL));
}

//Calculate the hright of the pixmap from the file size
int getHeight(int file_size, int width)
{
	return ceil(((float)file_size + sizeof(btbHeader)) / (width * BYTES_PER_PIXEL));
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
		if (errno == 2)
			fprintf(stderr, "Error in convertToBmp %i - %s \"%s\"\n", errno, strerror(errno), source);
		else
			fprintf(stderr, "Error in convertToBmp %i - %s\n", errno, strerror(errno));
		return -2;
	}
	
	//Get size of file to convert
	int filesize = fsize(binary);
	
	//Calculate the width and height of the pixmap 
	int side_width = getWidth(filesize);
	
	int side_height = getHeight(filesize, side_width);
	
#ifdef _DEBUG
	printf("Pixmap size: %i x %i\n", side_width, side_height);
#endif
	
	//Calculate padding required for bitmap
	long long pixmap_size = (long long)side_width * (long long)side_height * BYTES_PER_PIXEL;
	
	btbHeader.paddingSize = pixmap_size - filesize - sizeof(btbHeader);
	
#ifdef _DEBUG
	printf("Padding size: %i\n", btbHeader.paddingSize);
	printf("File size: %i\n", filesize);
#endif
	
	//Extend file if smaller than sizeof(completeHeader)
	if (filesize < sizeof(completeHeader))		
	{
		fclose(binary);
		if (resizefile(source, sizeof(completeHeader)))
		{
			return 1;
		}
		fopen(source, "rb+");
	}
	
	//Copy the beginning bytes (size of File_Head) from original file into memory
	fread(&completeHeader, 1, sizeof(completeHeader), binary);
	if (ferror(binary))
	{
		fprintf(stderr, "Error: Could not read first block for\n");
		return 1;
	}
	
	//Append the copied bytes to file
	fseek(binary, 0, SEEK_END);
	
	fwrite(&completeHeader,  sizeof(completeHeader), 1, binary);
	if (ferror(binary))
	{
		fprintf(stderr, "Error: Could not append first block\n");
		return 1;
	}
	
#ifdef _DEBUG
	printf("Head file: %i\n", (int)sizeof(completeHeader));
#endif
	
	//Modify BMP template header
	Bitmap_Header.fileSize = pixmap_size + sizeof(Bitmap_Header);
	Bitmap_Header.width = side_width;
	Bitmap_Header.height = side_height;
	Bitmap_Header.pixmapSize = pixmap_size;
	
	//Go to beginning of stream
	fseek(binary, 0, SEEK_SET);
	
	//Write BMP header to file
	fwrite(&Bitmap_Header, sizeof(Bitmap_Header), 1, binary);

	//Write size of padding to bitmap in first pixel
	fwrite(&btbHeader, sizeof(btbHeader), 1, binary);
	
	if (ferror(binary))
	{
		fprintf(stderr, "Error: Could not write BMP or BTB headers\n");
		return 1;
	}
		
	//Close write file
	fclose(binary);
	
	//Resize to add padding
	if (resizefile(source, pixmap_size + sizeof(Bitmap_Header)))
		return 1;
	
	//Add .bmp to filename
	if (appendfilename(source))
		return 1;
	
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
		if (errno == 2)
			fprintf(stderr, "Error in convertToBinary %i - %s \"%s\"\n", errno, strerror(errno), source);
		else
			fprintf(stderr, "Error in convertToBinary %i - %s\n", errno, strerror(errno));
		return -1;
	}	
	
	//Load bitmap header into memory
	int filesize = fsize(bitmap);
	
	fread(&completeHeader, sizeof(completeHeader), 1, bitmap);
	
	if (ferror(bitmap))
	{
		fprintf(stderr, "Error: Could not read bitmap header\n");
		return 1;
	}
	
	//Get the size of padding
	int padding_size = completeHeader.btb.paddingSize;
	
#ifdef _DEBUG
	printf("Padding: %i\n", padding_size);
#endif
		
	//Perform some tests to make sure file is valid BTB bitmap
	//		Check first few characters are BM
	if (completeHeader.bmp.ID != Bitmap_Header.ID)
	{
		fprintf(stderr, "Error: Invalid bitmap file (No BM signature)\n");
		return 1;
	}
	
	//		Check the DIB size is correct, as toBMP uses only Bitmap header v4
	if (completeHeader.bmp.DIBSize != Bitmap_Header.DIBSize)
	{
		fprintf(stderr, "Error: Bitmap file not supported\n");
		return 1;
	}
	
	//		Check padding isnt larger than pixmap
	if (padding_size >= completeHeader.bmp.pixmapSize)
	{
		fprintf(stderr, "Error: Invalid bitmap file (bad padding value)\n");
		return 1;
	}

	//		Check signature
	if (memcmp(&completeHeader.btb.signature, &btbHeader.signature, sizeof(btbHeader.signature)))
	{
		fprintf(stderr, "Error: Invalid bitmap file (bad BTB signature)\n");
		return 1;
	}
	
	//		Check to make sure width, height, bpp, pixmap size, bmp and dib header size and actual size all add up
	
	int calculatedSize = completeHeader.bmp.width * completeHeader.bmp.height * (completeHeader.bmp.bpp / 8) + completeHeader.bmp.DIBSize + 14;
	
	if (calculatedSize != filesize || completeHeader.bmp.fileSize != filesize)
	{
		fprintf(stderr, "Error: Invalid bitmap file (bad bitmap metadata)\n");
		return 1;
	}
	
	
	//Get the location of the head and  the size of the original file
	int headLocation = filesize - padding_size - sizeof(completeHeader);
	int originalSize = headLocation;
	
	if (originalSize < sizeof(completeHeader)) //If the original size is smaller than the header, then the headLocation must be at the end of the completeHeader
		headLocation = sizeof(completeHeader);
	
#ifdef _DEBUG
	printf("Head Location: %i\n", headLocation);
#endif
	
	//Go to the head
	fseek(bitmap, headLocation, SEEK_SET);
	
	//Read the head
	int bytesRead = fread(&completeHeader, 1, sizeof(completeHeader), bitmap);
	
	
	if (ferror(bitmap))
	{
		fprintf(stderr, "Error: Could not read head\n");
		return 1;
	}
	
#ifdef _DEBUG
	printf("Bytes read: %i\n", bytesRead);
#endif
	
	//Go to beginning
	fseek(bitmap, 0, SEEK_SET);
	
	//Write the head back to the beginning
	fwrite(&completeHeader, 1, bytesRead, bitmap);
	
	if (ferror(bitmap))
	{
		fprintf(stderr, "Error: Could not write head to beginning\n");
		return 1;
	}
	
	//Close file
	fclose(bitmap);
	
	//Truncate file to remove padding
	if (resizefile(source, originalSize))
		return 1;
	
	//Rename file
	if (truncatefilename(source))
		return 1;
	
	return 0;
}

int main(int argc, char **argv)
{	
	char* source;
	
	if (argc == 1) //No command line arguments supplied, error.
	{
		fprintf(stderr, "Error: No input file. Program terminated.\n");
		return 1;
	}
	else if (argc == 2) //One argument supplied.
	{
		source = argv[1];
	}
	else if (argc >= 3) // two args. Invalid
	{		
		fprintf(stderr, "Error: Only a single argument is accepted.\n");
	}	
	
#ifdef _DEBUG
	printf("Source     : %s\n", source);
#endif
	
	//Get last 4 characters of source as string
	char* sub = &source[strlen(source) - 4];
	
	int result;
	
	if (strcmp(sub, ".bmp") == 0)
	{
		printf("Direction  : Bmp to Bin\n");
		result = convertToBinary(source);
	}
	else
	{
		printf("Direction  : Bin to Bmp\n");
		result = convertToBmp(source);
	}
	
	if (result)
	{
		fprintf(stderr, "Conversion failed.\n");
		return 1;
	}
		
	printf("Conversion complete\n");
	
	
	return 0;
}

