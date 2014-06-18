#include <fstream>

#include <vigra/impex.hxx>

#include <util/Logger.h>
#include "ImageFileReader.h"

logger::LogChannel imagefilereaderlog("imagefilereaderlog", "[ImageFileReader] ");

ImageFileReader::ImageFileReader(std::string filename) :
	_filename(filename) {
}


void
ImageFileReader::readImage() {

	// get information about the image to read
	vigra::ImageImportInfo info(_filename.c_str());

	// abort if image is not grayscale
	if (!info.isGrayscale()) {

		LOG_ERROR(imagefilereaderlog) << _filename << " is not a gray-scale image!" << std::endl;
		return;
	}

	// allocate image
	_image = new Image(info.width(), info.height());

	// read image
	importImage(info, vigra::destImage(*_image));

	if (strcmp(info.getPixelType(), "FLOAT") == 0)
		return;

	// scale image to [0..1]

	float factor = 1.0;
	if (strcmp(info.getPixelType(), "UINT8") == 0)
		factor = 255.0;
	else if (strcmp(info.getPixelType(), "INT16") == 0)
		factor = 511.0;
	else {

		LOG_ERROR(imagefilereaderlog) << _filename << " has a unsupported pixel format: " << info.getPixelType() << std::endl;
	}

	vigra::transformImage(
			vigra::srcImageRange(*_image),
			vigra::destImage(*_image),
			vigra::linearIntensityTransform<float>(1.0/factor));
}
