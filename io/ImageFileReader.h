#ifndef PIPELINE_IMAGE_FILE_READER_H__
#define PIPELINE_IMAGE_FILE_READER_H__

#include <pipeline/all.h>
#include <imageprocessing/Image.h>
#include "ImageReader.h"

class ImageFileReader : public ImageReader {

public:

	ImageFileReader(std::string filename);
	
protected:
    /**
	 * Reads the image.
	 */
	void readImage();

private:
	// the name of the file to read
	std::string _filename;
};

#endif // PIPELINE_IMAGE_FILE_READER_H__

