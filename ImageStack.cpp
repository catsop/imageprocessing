#include "ImageStack.h"

template <typename ImageType>
void
ImageStack<ImageType>::clear() {

	_sections.clear();
}

template <typename ImageType>
void
ImageStack<ImageType>::add(boost::shared_ptr<ImageType> section) {

	_sections.push_back(section);
}

template <typename ImageType>
void
ImageStack<ImageType>::addAll(boost::shared_ptr<ImageStack<ImageType> > sections) {

	_sections.insert(_sections.end(), sections->begin(), sections->end());
}

EXPLICITLY_INSTANTIATE_COMMON_IMAGE_TYPES(ImageStack);
