#include <vigra/impex.hxx>
#include <vigra/impexalpha.hxx>

#include <util/Logger.h>
#include "ImageWriter.h"

logger::LogChannel imagewriterlog("imagewriterlog", "[ImageWriter] ");

template <typename ImageType>
ImageWriter<ImageType>::ImageWriter(std::string filename) :
	_filename(filename) {

	// let others know about our inputs
	registerInput(_image, "image");
}

template <typename ImageType>
void
ImageWriter<ImageType>::write(std::string filename) {

	LOG_DEBUG(imagewriterlog) << "requesting image update" << std::endl;

	updateInputs();

	if (!_image.isSet()) {

		LOG_ERROR(imagewriterlog) << "no input image set" << std::endl;
		return;
	}

	if (filename == "")
		filename = _filename;

	LOG_DEBUG(imagewriterlog) << "attempting to write image" << std::endl;

	// save to file
	writeToFile(filename);

	LOG_DEBUG(imagewriterlog) << "image written" << std::endl;
}

template <>
void
ImageWriter<IntensityImage>::writeToFile(std::string filename) {

	vigra::exportImage(vigra::srcImageRange(*_image), vigra::ImageExportInfo(filename.c_str()));
}

template <>
void
ImageWriter<LabelImage>::writeToFile(std::string filename) {

	vigra::MultiArray<2, vigra::RGBValue<vigra::UInt16> > intensity(vigra::Shape2(_image->width(), _image->height()));
	vigra::MultiArray<2, vigra::UInt16> alpha(vigra::Shape2(_image->width(), _image->height()));

	// See endian explanation in ImageWriter implementation.

	for (int i = 0; i < _image->size(); ++i) {

		LabelImage::value_type label = (*_image)[i];
		unsigned short *label_shorts = reinterpret_cast<unsigned short *>(&label);
		intensity[i].setRed(label_shorts[LABEL_IMAGE_RGBA_ORDER[0]]);
		intensity[i].setGreen(label_shorts[LABEL_IMAGE_RGBA_ORDER[1]]);
		intensity[i].setBlue(label_shorts[LABEL_IMAGE_RGBA_ORDER[2]]);
		alpha[i] = label_shorts[LABEL_IMAGE_RGBA_ORDER[3]];
	}

	vigra::exportImageAlpha(intensity, alpha, vigra::ImageExportInfo(filename.c_str()).setPixelType("UINT16"));
}

EXPLICITLY_INSTANTIATE_COMMON_IMAGE_TYPES(ImageWriter);
