#include <boost/lexical_cast.hpp>

#include <imageprocessing/ImageStack.h>
#include <imageprocessing/Image.h>
#include "ImageExtractor.h"

static logger::LogChannel imageextractorlog("imageextractorlog", "[ImageExtractor] ");

template <typename ImageType>
ImageExtractor<ImageType>::ImageExtractor() {

	registerInput(_stack, "stack");

	_stack.registerCallback(&ImageExtractor::onInputSet, this);
}

template <typename ImageType>
void
ImageExtractor<ImageType>::onInputSet(const pipeline::InputSetBase&) {

	LOG_ALL(imageextractorlog) << "input image stack set" << std::endl;

	updateInputs();

	_images.resize(_stack->size());

	// for each image in the stack
	for (unsigned int i = 0; i < _stack->size(); i++) {

		LOG_ALL(imageextractorlog) << "(re)setting output " << i << std::endl;

		_images[i] = (*_stack)[i];

		LOG_ALL(imageextractorlog) << "image is " << _images[i]->width() << "x" << _images[i]->height() << std::endl;

		LOG_ALL(imageextractorlog) << "registering output 'image " << i << "'" << std::endl;

		registerOutput(_images[i], "image " + boost::lexical_cast<std::string>(i));
	}
}

template <typename ImageType>
void
ImageExtractor<ImageType>::updateOutputs() {

	// there is nothing to do here
}

template <typename ImageType>
unsigned int
ImageExtractor<ImageType>::getNumImages() {

	return _images.size();
}

EXPLICITLY_INSTANTIATE_COMMON_IMAGE_TYPES(ImageExtractor);
