#ifndef IMAGE_BLOCK_READER_H__
#define IMAGE_BLOCK_READER_H__

#include "ImageReader.h"
#include <util/box.hpp>
#include <pipeline/all.h>

template <typename ImageType>
class ImageBlockReader : public ImageReader<ImageType> {

protected:
    ImageBlockReader();

    using ImageBlockReader::ImageReader::registerInput;

    using ImageBlockReader::ImageReader::_image;

    pipeline::Input<util::box<unsigned int, 3> > _block;

	pipeline::Input<unsigned int> _sectionId;
    //unsigned int _sectionId;

};

#endif //IMAGE_BLOCK_READER_H__
