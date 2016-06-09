#ifndef IMAGE_BLOCK_FACTORY_H__
#define IMAGE_BLOCK_FACTORY_H__

#include <imageprocessing/io/ImageBlockReader.h>
#include <boost/shared_ptr.hpp>
#include <pipeline/Data.h>

template <typename ImageType>
class ImageBlockFactory : public pipeline::Data
{
public:
	virtual boost::shared_ptr<ImageBlockReader<ImageType> > getReader(int /*n*/) {
		return boost::shared_ptr<ImageBlockReader<ImageType> >();
	}
};

#endif //IMAGE_BLOCK_FACTORY_H__
