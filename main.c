
#include <stdio.h> //Printf, FILE

#include <errno.h> //Error numbers for file handling

#include <string.h> //String handling

#include <stdlib.h> //Malloc

#include <math.h> //log, ldexp

// Hint: a file size of 16765487 bytes will convert to an image under 16MB, perfect for google photo storage!

//Array of bytes containing bitmap header
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

//Enumerates the nature of the conversion
enum Direction { BinToBmp, BmpToBin };

//Enumerates whether or not the user is to be prompted for input
enum Prompt { Loud, Quiet };

//Structure containing the options used 
struct Options
{
	enum Direction direct;
	enum Prompt prompt;
} _args = { BinToBmp, Loud };
	
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
int getSide(int file_size)
{
	int n = ceil(sqrt(file_size / 3.0 + 1.0));
	n = n + (4 - n % 4) % 4; //Pad the width and height to make it divisible by 4.
	return n;
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
int convertToBmp(const char* source, const char* destination)
{
	printf("Converting from binary to bmp\n");
	
	//Open input file to read
	FILE* binary;
	
	binary = fopen(source, "rb");
	
	if (binary == NULL)	
	{
		printf("Error in convertToBmp %i - %s\n", errno, strerror(errno));
		return -1;
	}
	
	//Get size of read file
	int filesize = fsize(binary);
	
	//Copy read file's contents to memory
	int side_length = getSide(filesize);
	
	char* binary_data = (char*)malloc(side_length * side_length * 3); //Create array to fit entire pixmap
	
	if (binary_data == NULL)
		return -3;
	
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
	
	//Calculate the total padding size 
	int padding = (long long)side_length * (long long)side_length * 3 - filesize;
	
	printf("Padding size: %i\n", padding);
	
	//Modify header
	setHeader(side_length, side_length);
	
	//Write header to file
	fwrite(Bitmap_Header, sizeof(*binary_data), sizeof(Bitmap_Header), bitmap);
	
	//Write size of original file to bitmap in first pixel
	fwrite(&padding, 3, 1, bitmap);
	
	//Write contents of read file to remaining pixels
	fwrite(binary_data, sizeof(*binary_data), side_length * side_length * 3 - 3, binary);
		
	//Close write file
	fclose(bitmap);
	
	return 0;
}

//Convert source bitmap back to binary file in destination
int convertToBinary(const char* source, const char* destination)
{
	printf("Convert bmp to binary file\n");
	
	FILE* bitmap;
	
	//Open bitmap file
	bitmap = fopen(source, "rb");
	
	if (bitmap == NULL)	
	{
		printf("Error in convertToBinary %i - %s\n", errno, strerror(errno));
		return -1;
	}	
	
	//Load file into memory
	int filesize = fsize(bitmap);
	
	char* data = (char*)malloc(filesize);
	
	fread(data, sizeof(*data), filesize, bitmap);
	
	fclose(bitmap);
	
	//Get starting address of bitmap data (as per the bitmap header standard)
	int address = *(int*)&data[10];
	
	//Go to the first pixel, get the size of padding. Use this to calculate the size of the original file
	int padding_size = (unsigned char)data[address] + (unsigned char)data[address + 1] * 256 + (unsigned char)data[address + 2] * 65536; //wouldn't it be best to cast first 4 bytes to int, then and with 0xffffff?
	
	int original_size = filesize - padding_size - 54;
	
	//Save data to file
	FILE* binary;
	
	binary = fopen(destination, "wb");
	
	fwrite(&data[address + 3], sizeof(*data), original_size, binary); //address + 3 to skip first three bytes (which is padding information
	
	fclose(binary);
	
	return 0;
}

int isOption(const char* arg)
{
	return (int)(arg[0] == '-');
}

//If no second path is supplied, create one by appending or removing *.bmp
char* createName(const char* source)
{
	char* new_file_name;
	
	if (_args.direct == BinToBmp)
	{
		new_file_name = (char*)malloc(strlen(source) + 4);
	
		if (new_file_name == NULL)
			return NULL;
		
		strcpy(new_file_name, source);
		
		strcat(new_file_name, ".bmp");
	}
	else
	{
		new_file_name = (char*)malloc(strlen(source) - 3);
		
		const char* sub = strstr(source, ".bmp");
		
		if (sub == NULL)
		{
			printf("%s is not a valid bitmap, was not been created by tobmp, or does not exist.\n", source);
			return NULL;
		}
		
		strncpy(new_file_name, source, (int)(sub - source));
		
		new_file_name[strlen(source) - 4] = '\0';
		
	}
	
	return new_file_name;
}

int  setOption(const char* option)
{
	if (strcmp(option, "-bmp") == 0)
	{
		_args.direct = BinToBmp;
	}
	else if (strcmp(option, "-bin") == 0)
	{
		_args.direct = BmpToBin;
	}
	else if (strcmp(option, "-q") == 0)
	{
		_args.prompt = Quiet;
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
	char* destination;
	
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
		destination = createName(source);
		
	}
	else if (argc >= 3) // two args. either 2 paths, or one option one path.
	{
		int pathcount = 0;
				
		for (int i = argc - 1; i >= 1; --i)
		{
			if (!isOption(argv[i]))
			{
				if (pathcount == 0)
					source = argv[i];
				else if (pathcount == 1)
					destination = argv[i];
				else if (pathcount > 1)
				{
					printf("Invalid argument %s.\n", argv[i]);
				}
				pathcount++;
			}
			else
			{
				setOption(argv[i]);
			}
		}
		
		if (pathcount == 1)
			destination = createName(source);		
	}	
	
	//createName() returns null if any of its memory allocations return null
	if (destination == NULL)
	{
		printf("Memory allocation error\n");
		return 0;
	}
	
	if (check_exists(destination) && _args.prompt == Loud)
	{
		char buff[15];
		printf("The file %s already exists. Would you like to continue(Y/N)? If you do the file will be overwritten.\n", destination);
		scanf("%s", buff);
		if (strcmp(buff, "y") != 0 && strcmp(buff, "Y") != 0)
			return 0;
	}
	
	printf("Direction  : %s\n", (_args.direct == BinToBmp ? "Bin to Bmp" : "Bmp to Bin"));
	printf("Prompt     : %s\n", (_args.prompt == Quiet ? "Quiet" : "Loud"));
	printf("Source     : %s\n", source);
	printf("Destination: %s\n", destination);
	
	if (_args.direct == BinToBmp)
		convertToBmp(source, destination);
	else
		convertToBinary(source, destination);
		
	printf("Conversion complete\n");
	
	
	return 0;
}

