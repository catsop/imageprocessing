#include <util/Logger.h>
#include <util/exceptions.h>
#include "ImageReader.h"

logger::LogChannel imagereaderlog("imagereaderlog", "[ImageReader] ");

ImageReader::ImageReader()
{
	// let others know about our output
	registerOutput(_image, "image");
}

void
ImageReader::updateOutputs() {
	LOG_DEBUG(imagereaderlog) << "Updating outputs" << std::endl;
	readImage();
}
