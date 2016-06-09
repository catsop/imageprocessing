#include <util/Logger.h>
#include <util/exceptions.h>
#include "ImageReader.h"

logger::LogChannel imagereaderlog("imagereaderlog", "[ImageReader] ");

template <typename ImageType>
ImageReader<ImageType>::ImageReader()
{
	// let others know about our output
	registerOutput(_image, "image");
}

template <typename ImageType>
void
ImageReader<ImageType>::updateOutputs() {
	LOG_DEBUG(imagereaderlog) << "Updating outputs" << std::endl;
	readImage();
}

template class ImageReader<IntensityImage>;
