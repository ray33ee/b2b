
#include <stdio.h> //Printf, FILE, rename

#include <errno.h> //Error numbers for file handling

#include <string.h> //String handling

#include <math.h> //ceil, sqrt

#include <stdint.h> //uint32_t, uint16_t

//Platform specific headers, used for file truncation
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

//Error codes
const int INVALID_FILE_HANDLE				= -10;
const int SET_FILE_POINTER_ERROR			= -11;
const int SET_END_OF_FILE_ERROR				= -12;
const int CLOSE_HANDLE_ERROR				= -13;
const int LINUX_TRUNCATE_ERROR				= -14;
const int APPEND_FILE_NAME_ERROR			= -15;
const int OPEN_FILE_ERROR					= -16;
const int READ_DATA_ERROR					= -17;
const int WRITE_DATA_ERROR					= -18;
const int READ_HEADER_ERROR					= -19;
const int WRITE_HEADER_ERROR				= -20;
const int READ_BITMAP_ERROR					= -21;
const int INVALID_BITMAP_BM					= -22;
const int INVALID_BITMAP_PADDING_VALUE		= -23;
const int INVALID_BITMAP_BTB				= -24;
const int INVALID_BITMAP_METADATA			= -25;
const int NO_INPUT_FILE_ARGUMENT			= -26;
const int TOO_MANY_ARGUMENTS				= -27;
const int TRUNCATE_FILE_NAME_ERROR			= -28;	
const int WIP_INVALID_BITMAP_HEADER 		= -29;


#define BYTES_PER_PIXEL (4) // The number of bytes per pixel
#define BMP_HEADER_SIZE (14) //Size of the actual BMP header (NOT the DIB header) in bytes
#define MAX_STRING_SIZE (150) //Since all string handling is performed on static array allocated strings, a variable containing the max string size is declared

char buffer[MAX_STRING_SIZE]; //Buffer for string operations

//Force compiler to use 2 byte packing
#pragma pack(2)
struct BitmapV5Header
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
	uint32_t intent;
	uint32_t profileData;
	uint32_t profileSize;
	uint32_t reserved;
	
}
//Template header
Bitmap_Header = { 0x4D42, 0, 0, sizeof(struct BitmapV5Header), sizeof(struct BitmapV5Header) - BMP_HEADER_SIZE, 0, 0, 1, BYTES_PER_PIXEL * 8, 3, 0, 4000, 4000, 0, 0, 0xFF0000, 0xFF00, 0xFF, 0xFF000000, 0x57696E20, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 0, };

//Header containing information specific to the toBmp conversion
struct BTBHeader
{
	uint32_t paddingSize; 	//Size of padding, in bytes
	uint32_t signature[4];	//4 pixel signature, stamped into every bitmap
	uint32_t originalHeaderSize; //Size of DIB header and BMP header when the bitmap was created with toBMP. NOTE: We cannot use the entry in the actual header for this, as it could have changed if converted to a different BMP type. If this is the case, the original BMP header size must be known in order to recover the data.
} btbHeader = { 0, {0x6FAFEC0D,  0x7EF10C44, 0x68E85B0B, 0x9C0FB9EF }, sizeof(Bitmap_Header) };

//Force compile time assertion if BitmapV5Header and Bitmap_Header are different sizes
/*#define CASSERT(predicate, file) _impl_CASSERT_LINE(predicate,__LINE__,file)
#define _impl_PASTE(a,b) a##b
#define _impl_CASSERT_LINE(predicate, line, file) \
    typedef char _impl_PASTE(assertion_failed_##file##_,line)[2*!!(predicate)-1];
CASSERT(sizeof(BitmapV5Header) != sizeof(Bitmap_Header), BitmapV5Header_and_Bitmap_Header_structs_must_be_the_same_size);
*/

//Head of file is used to store beginning of original file (when converting to bmp) or bitmap header and btb header (when converting back to binary)
#pragma pack(2)
struct CompleteHeader
{
	struct BitmapV5Header bmp;
	struct BTBHeader btb;
} completeHeader;

struct JsonData
{
	char* source;
	char* destination;
	char* direction;
	int width;
	int height;
	int bpp;
	int filesize;
	int padding;
} jsonData;

// In order to comply with the JSON format, we escape our back slash characters
void printEscapedPath(char* path)
{
	while (*path != '\0')
	{
		if (*path == '\\')
			printf("\\\\");
		else
			printf("%c", *path);
		
		path++;
	}
}

// Output jsonData struct as json string to stdout
void printJSON()
{
	
	printf("{\n    \"source\": \"");
	printEscapedPath(jsonData.source);
	printf("\",\n");
	
	printf("    \"destination\": \"");
	printEscapedPath(jsonData.destination);
	printf("\",\n");
	
	printf("    \"destination\": \"%s\",\n", jsonData.destination);
	printf("    \"output\": \"%s\",\n", jsonData.direction);
	printf("    \"pixmap\": {\n        \"width\": %i,\n        \"height\":  %i,\n        \"bpp\": %i\n    },\n", jsonData.width, jsonData.height, (BYTES_PER_PIXEL * 8));
	printf("    \"filesize\": %i,\n", jsonData.filesize);
	printf("    \"headersize\": %i,\n", sizeof(struct BitmapV5Header));
	printf("    \"padding\": %i\n", jsonData.padding);
	printf("}");
}

//Platform independent function to resize file at path to size bytes
int resizefile(const char* path, int size)
{
#ifdef _WIN32
	//Get file handle for path
	HANDLE file = CreateFileA(path, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	
	if (file == INVALID_HANDLE_VALUE)
	{
		fprintf(stderr, "Error: Windows API returned error code %li. Visit 'https://docs.microsoft.com/en-us/windows/win32/debug/system-error-codes' for the error code description.\n", GetLastError());
		return INVALID_FILE_HANDLE;
	}
	
	//Set file pointer to new file size
	if (SetFilePointer(file, size, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
	{
		fprintf(stderr, "Error: Windows API returned error code %li. Visit 'https://docs.microsoft.com/en-us/windows/win32/debug/system-error-codes' for the error code description.\n", GetLastError());
		return SET_FILE_POINTER_ERROR;
	}
	
	//Set end of file to position of file pointer
	if (!SetEndOfFile(file))
	{
		fprintf(stderr, "Error: Windows API returned error code %li. Visit 'https://docs.microsoft.com/en-us/windows/win32/debug/system-error-codes' for the error code description.\n", GetLastError());
		return SET_END_OF_FILE_ERROR;
	}
	
	//Close file
	if (!CloseHandle(file))
	{
		fprintf(stderr, "Error: Windows API returned error code %li. Visit 'https://docs.microsoft.com/en-us/windows/win32/debug/system-error-codes' for the error code description.\n", GetLastError());
		return CLOSE_HANDLE_ERROR;
	}

#else
	if (truncate(path, size))
	{
		fprintf(stderr, "Error: Linux call to truncate() failed with error code %i - %s\n", errno, strerror(errno));
		return LINUX_TRUNCATE_ERROR;
	}
	
#endif

	return 0;

}

int appendfilename(const char* path)
{
	strcpy(buffer, path);
	strcat(buffer, ".bmp");
	
	if (rename(path, buffer))
	{
		fprintf(stderr, "Error: Could not append file name. Error %i - %s\n", errno, strerror(errno));
		return APPEND_FILE_NAME_ERROR;
	}
	
	return 0;
	
}

int truncatefilename(const char* path)
{
	strcpy(buffer, path);
	
	buffer[strlen(path) - 4] = '\0'; //Truncate a string the fun way!	
	
	if (rename(path, buffer))
	{
		fprintf(stderr, "Error: Could not truncate file name. Error %i - %s\n", errno, strerror(errno));
		return TRUNCATE_FILE_NAME_ERROR;
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
	//Open input file to read
	FILE* binary;
	
	int result;
	
	binary = fopen(source, "rb+");
	
	if (binary == NULL)	
	{
		if (errno == 2)
			fprintf(stderr, "Error in convertToBmp %i - %s \"%s\"\n", errno, strerror(errno), source);
		else
			fprintf(stderr, "Error in convertToBmp %i - %s\n", errno, strerror(errno));
		return OPEN_FILE_ERROR;
	}
	
	//Get size of file to convert
	jsonData.filesize = fsize(binary);
	
	//Calculate the width and height of the pixmap 
	jsonData.width = getWidth(jsonData.filesize);
	
	jsonData.height = getHeight(jsonData.filesize, jsonData.width);
	
	//Calculate padding required for bitmap
	long long pixmap_size = (long long)jsonData.width * (long long)jsonData.height * BYTES_PER_PIXEL;
	
	btbHeader.paddingSize = pixmap_size - jsonData.filesize - sizeof(btbHeader);
	
	jsonData.padding = btbHeader.paddingSize;
	
	//Extend file if smaller than sizeof(completeHeader)
	if (jsonData.filesize < sizeof(completeHeader))		
	{
		fclose(binary);
		result = resizefile(source, sizeof(completeHeader));
		if (result)
			return result;
		fopen(source, "rb+");
	}
	
	//Copy the beginning bytes (size of File_Head) from original file into memory
	fread(&completeHeader, 1, sizeof(completeHeader), binary);
	if (ferror(binary))
	{
		fprintf(stderr, "Error: Could not read first block for\n");
		return READ_DATA_ERROR;
	}
	
	//Append the copied bytes to file
	fseek(binary, 0, SEEK_END);
	
	fwrite(&completeHeader,  sizeof(completeHeader), 1, binary);
	if (ferror(binary))
	{
		fprintf(stderr, "Error: Could not append first block\n");
		return READ_DATA_ERROR;
	}
	
	
	//Modify BMP template header
	Bitmap_Header.fileSize = pixmap_size + sizeof(Bitmap_Header);
	Bitmap_Header.width = jsonData.width;
	Bitmap_Header.height = jsonData.height;
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
		return WRITE_DATA_ERROR;
	}
		
	//Close write file
	fclose(binary);
	
	//Resize to add padding
	result = resizefile(source, pixmap_size + sizeof(Bitmap_Header));
	if (result)
		return result;
	
	//Add .bmp to filename
	result = appendfilename(source);
	if (result)
		return result;
	
	return 0;
}

//Convert source bitmap back to binary file in destination
int convertToBinary(const char* source)
{	
	FILE* bitmap;
	
	int result;
	
	//Open bitmap file
	bitmap = fopen(source, "rb+");
	
	if (bitmap == NULL)	
	{
		if (errno == 2)
			fprintf(stderr, "Error in convertToBinary %i - %s \"%s\"\n", errno, strerror(errno), source);
		else
			fprintf(stderr, "Error in convertToBinary %i - %s\n", errno, strerror(errno));
		return OPEN_FILE_ERROR;
	}	
	
	//Load bitmap header into memory
	jsonData.filesize = fsize(bitmap);
	
	fread(&completeHeader, 1, sizeof(completeHeader), bitmap);
	
	if (ferror(bitmap))
	{
		fprintf(stderr, "Error: Could not read bitmap header\n");
		return READ_HEADER_ERROR;
	}
	
	//Get the size of padding
	jsonData.padding = completeHeader.btb.paddingSize;
		
	//Perform some tests to make sure file is valid BTB bitmap
	//		Check first few characters are BM
	if (completeHeader.bmp.ID != Bitmap_Header.ID)
	{
		fprintf(stderr, "Error: Invalid bitmap file (No BM signature)\n");
		return INVALID_BITMAP_BM;
	}
	
	//		Check padding isnt larger than pixmap
	if (jsonData.padding >= completeHeader.bmp.pixmapSize)
	{
		fprintf(stderr, "Error: Invalid bitmap file (bad padding value)\n");
		return INVALID_BITMAP_PADDING_VALUE;
	}

	//		Check signature
	if (memcmp(&completeHeader.btb.signature, &btbHeader.signature, sizeof(btbHeader.signature)))
	{
		fprintf(stderr, "Error: Invalid bitmap file (bad BTB signature)\n");
		return INVALID_BITMAP_BTB;
	}
	
	//		Compare the original header size (the size of the header of the bitmap created directly by tobmp). If different, another program has converted the bitmap to a different kind.
	if (completeHeader.bmp.offset != btbHeader.originalHeaderSize)
	{
		fprintf(stderr, "Error: Only Bitmap V5 is currently supported (Invalid DIB header size)\n");
		
		if (completeHeader.bmp.DIBSize < btbHeader.originalHeaderSize)
		{
			//New header is smaller than original, so file must be truncated
		}
		else
		{ 
			//New header is larger than original, so file must be extended
		}
		
		return WIP_INVALID_BITMAP_HEADER;
	}
	
	jsonData.width = completeHeader.bmp.width;
	jsonData.height = completeHeader.bmp.height;
	
	//		Check to make sure width, height, bpp, pixmap size, bmp and dib header size and actual size all agree
	
	int calculatedSize = completeHeader.bmp.width * completeHeader.bmp.height * (completeHeader.bmp.bpp / 8) + completeHeader.bmp.DIBSize + BMP_HEADER_SIZE;	
	
	if (calculatedSize != jsonData.filesize || completeHeader.bmp.fileSize != jsonData.filesize)
	{
		fprintf(stderr, "Error: Invalid bitmap file (bad bitmap metadata)\n");
		return INVALID_BITMAP_METADATA;
	}
	
	
	//Get the location of the head and  the size of the original file
	int headLocation = jsonData.filesize - jsonData.padding - btbHeader.originalHeaderSize - sizeof(struct BTBHeader);
	int originalSize = jsonData.filesize - jsonData.padding - completeHeader.bmp.offset - sizeof(struct BTBHeader);
	
	if (originalSize < btbHeader.originalHeaderSize + sizeof(struct BTBHeader)) //If the original size is smaller than the header, then the headLocation must be at the end of the completeHeader
	headLocation = completeHeader.bmp.offset + sizeof(struct BTBHeader);
	
	//Go to the head
	fseek(bitmap, headLocation, SEEK_SET);
	
	//Read the head
	int bytesRead = fread(&completeHeader, 1, btbHeader.originalHeaderSize + sizeof(struct BTBHeader), bitmap);
	
	
	if (ferror(bitmap))
	{
		fprintf(stderr, "Error: Could not read head\n");
		return READ_HEADER_ERROR;
	}
	
	//Go to beginning
	fseek(bitmap, 0, SEEK_SET);
	
	//Write the head back to the beginning
	fwrite(&completeHeader, 1, bytesRead, bitmap);
	
	if (ferror(bitmap))
	{
		fprintf(stderr, "Error: Could not write head to beginning\n");
		return WRITE_HEADER_ERROR;
	}
	
	//Close file
	fclose(bitmap);
	
	//Truncate file to remove padding
	result = resizefile(source, originalSize);
	if (result)
		return result;
	
	//Rename file
	result = truncatefilename(source);
	if (result)
		return result;
	
	return 0;
}

int main(int argc, char **argv)
{		
	
	jsonData.destination = "";
		
	if (argc == 1) //No command line arguments supplied, error.
	{
		fprintf(stderr, "Error: No input file. Program terminated.\n");
		return NO_INPUT_FILE_ARGUMENT;
	}
	else if (argc == 2) //One argument supplied.
	{
		jsonData.source = argv[1];
	}
	else if (argc >= 3) // two args. Invalid
	{		
		fprintf(stderr, "Error: Only a single argument is accepted.\n");
		return TOO_MANY_ARGUMENTS;
	}

	
	//Get last 4 characters of source as string
	char* sub = &jsonData.source[strlen(jsonData.source) - 4];
	
	int result;
	
	if (strcmp(sub, ".bmp") == 0)
	{
		jsonData.direction = "binary";
		result = convertToBinary(jsonData.source);
	}
	else
	{
		jsonData.direction = "bitmap";
		result = convertToBmp(jsonData.source);
	}
	
	if (result)
	{
		fprintf(stderr, "Conversion failed.\n");
		return result;
	}
	
	printJSON();
	
	
	return 0;
}

