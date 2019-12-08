# toBMP

The toBMP executable converts any binary file into a bitmap image. 

toBMP is able to convert from binary to bitmap, and back again. It does so by modifing small areas of the original file, so the conversion is very fast even for large files.

# Why toBMP?

toBMP is reliable, fast, small, cross-platform and extremely efficient. toBMP offers users a way to hide data in the form of images, upload to cloud services, and SOMETHING ELSE AMAZING. 

# What's the point?

toBMP was created as a fast, minimal and reliable tool to convert any binary file into a valid bitmap image. The original use of toBMP was as a tool to upload files to Google cloud storage without taking up storage. Google Photos allows users to upload images of up to 16MP (4000x4000) uncompressed, under 'high quality' without counting towards the users' storage space. Click [here]() for more information. Storing data as Base64 encoded text files was also explored, but files stored this way take an extra one third of data, increasing upload times. Converting to bitmap adds very little extra padding (the padding, excluding the bitmap header and toBMP metadata, will never exceed the width * 4).

# How does it work?

toBMP works by copying the first hundred or so bytes to the end of the file, then inserting a bitmap header at the beginning. Finally we add a little extra padding to the pixmap to ensure the file is a valid bitmap image. All data is stored in the pixmap, with pixels at 32bpp. None of the original file data is stored in the bitmap header.

To convert from binary to bitmap, simply run toBMP with the path of the file as an argument. To convert back, simply do the same with the converted file as the path. NOTE: if you try to input a bmp file that was not created with toBMP, the conversion will fail.

# Installation

Simply run make in the source directory to build toBMP.