#include <fstream>

#include <vigra/impex.hxx>
#include <vigra/impexalpha.hxx>

#include <util/Logger.h>
#include "ImageFileReader.h"
#include "ImageWriter.h"

logger::LogChannel imagefilereaderlog("imagefilereaderlog", "[ImageFileReader] ");

template <typename ImageType>
ImageFileReader<ImageType>::ImageFileReader(std::string filename) :
	_filename(filename) {
}


template <>
void
ImageFileReader<IntensityImage>::readImage() {

	// get information about the image to read
	vigra::ImageImportInfo info(_filename.c_str());

	// abort if image is not grayscale
	if (!info.isGrayscale()) {

		UTIL_THROW_EXCEPTION(
				IOError,
				_filename << " is not a gray-scale image!");
	}

	// allocate image
	_image = new IntensityImage(info.width(), info.height());
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

	IntensityImage::value_type factor;
	if (strcmp(info.getPixelType(), "UINT8") == 0)
		factor = 255.0;
	else {

		factor = 1.0;
		LOG_DEBUG(imagefilereaderlog) << _filename << " has pixel format " << info.getPixelType() << ", will not scale values" << std::endl;
	}

	vigra::transformImage(
			vigra::srcImageRange(*_image),
			vigra::destImage(*_image),
			vigra::linearIntensityTransform<IntensityImage::value_type>(1.0/factor));
}


template <>
void
ImageFileReader<LabelImage>::readImage() {

	// get information about the image to read
	vigra::ImageImportInfo info(_filename.c_str());

	// abort if image is not grayscale
	if (info.isGrayscale() || strcmp(info.getPixelType(), "UINT16") != 0) {

		UTIL_THROW_EXCEPTION(
				IOError,
				_filename << " is not a 16bpp RGBA image!");
	}

	// allocate image
	_image = new LabelImage(info.width(), info.height());
	_image->setIdentifiyer(_filename);

	vigra::MultiArray<2, vigra::RGBValue<vigra::UInt16> > intensity(vigra::Shape2(info.width(), info.height()));
	vigra::MultiArray<2, vigra::UInt16> alpha(vigra::Shape2(info.width(), info.height()));

	try {

		// read image
		importImageAlpha(info, intensity, alpha);

	} catch (vigra::PostconditionViolation& e) {

		UTIL_THROW_EXCEPTION(
				IOError,
				"error reading " << _filename << ": " << e.what());
	}

	for (int i = 0; i < _image->size(); ++i) {

		LabelImage::value_type label = 0;
		unsigned short *label_shorts = reinterpret_cast<unsigned short *>(&label);
		label_shorts[LABEL_IMAGE_RGBA_ORDER[0]] = intensity[i].red();
		label_shorts[LABEL_IMAGE_RGBA_ORDER[1]] = intensity[i].green();
		label_shorts[LABEL_IMAGE_RGBA_ORDER[2]] = intensity[i].blue();
		label_shorts[LABEL_IMAGE_RGBA_ORDER[3]] = alpha[i];
		(*_image)[i] = label;
	}
}

EXPLICITLY_INSTANTIATE_COMMON_IMAGE_TYPES(ImageFileReader);
