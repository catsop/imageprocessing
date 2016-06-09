#include "ImageBlockReader.h"

template <typename ImageType>
ImageBlockReader<ImageType>::ImageBlockReader()
{
    registerInput(_block, "block");
	registerInput(_sectionId, "section");
}