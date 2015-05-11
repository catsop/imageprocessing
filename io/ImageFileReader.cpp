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

		UTIL_THROW_EXCEPTION(
				IOError,
				_filename << " is not a gray-scale image!");
	}

	// allocate image
	_image = new Image(info.width(), info.height());
	_image->setIdentifiyer(_filename);

	try {

		// read image
		importImage(info, vigra::destImage(*_image));

	} catch (vigra::PostconditionViolation& e) {

		UTIL_THROW_EXCEPTION(
				IOError,
				"error reading " << _filename << ": " << e.what());
	}

	if (strcmp(info.getPixelType(), "FLOAT") == 0)
		return;

	// scale image to [0..1]

	float factor;
	if (strcmp(info.getPixelType(), "UINT8") == 0)
		factor = 255.0;
	else {

		factor = 1.0;
		LOG_DEBUG(imagereaderlog) << _filename << " has pixel format " << info.getPixelType() << ", will not scale values" << std::endl;
	}

	vigra::transformImage(
			vigra::srcImageRange(*_image),
			vigra::destImage(*_image),
			vigra::linearIntensityTransform<float>(1.0/factor));
}
