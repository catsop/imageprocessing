#include "ImageBlockFileReader.h"
#include <pipeline/Value.h>

logger::LogChannel imageblockfilereaderlog("imageblockfilereaderlog",
										    "[ImageBlockFileReader] ");

template <typename ImageType>
ImageBlockFileReader<ImageType>::ImageBlockFileReader(std::string filename)
{
    _fileReader = boost::make_shared<ImageFileReader<ImageType> >(filename);
    //_imageCrop = boost::make_shared<ImageCrop>();
    //_imageCrop->setInput("image", _fileReader->getInput("image"));
}

template <typename ImageType>
void
ImageBlockFileReader<ImageType>::readImage()
{
	LOG_DEBUG(imageblockfilereaderlog) << "reading cropped image" << std::endl;
	
    pipeline::Value<ImageType> cropped;
	pipeline::Value<int> x(_block->min().x());
    pipeline::Value<int> y(_block->min().y());
    pipeline::Value<int> w(_block->width());
    pipeline::Value<int> h(_block->height());

	_imageCrop = boost::make_shared<ImageCrop<ImageType> >();
	LOG_DEBUG(imageblockfilereaderlog) << "Created ImageCrop, setting things up" << std::endl;
    _imageCrop->setInput("image", _fileReader->getOutput("image"));
    _imageCrop->setInput("x", x);
    _imageCrop->setInput("y", y);
    _imageCrop->setInput("width", w);
    _imageCrop->setInput("height", h);

	LOG_DEBUG(imageblockfilereaderlog) << "Getting cropped output" << std::endl;
    cropped = _imageCrop->getOutput("cropped image");

	_image = new ImageType();
   
	LOG_DEBUG(imageblockfilereaderlog) << "Pointer magic" << std::endl;
    *_image = *cropped;
	LOG_DEBUG(imageblockfilereaderlog) << "done reading" << std::endl;
}

template <typename ImageType>
ImageBlockFileFactory<ImageType>::ImageBlockFileFactory(const std::string& directory)
{
	LOG_DEBUG(imageblockfilereaderlog) << "reading from directory " << directory << std::endl;
	
	boost::filesystem::path dir(directory);
	
	if (!boost::filesystem::exists(dir))
	{
		BOOST_THROW_EXCEPTION(IOError() << error_message(directory + " does not exist"));
	}
	
	if (!boost::filesystem::is_directory(dir))
	{
		BOOST_THROW_EXCEPTION(IOError() << error_message(directory + " is not a directory"));
	}
	
	std::copy(
			boost::filesystem::directory_iterator(dir),
			boost::filesystem::directory_iterator(),
			back_inserter(_sortedPaths));
	std::sort(_sortedPaths.begin(), _sortedPaths.end());
	
	LOG_DEBUG(imageblockfilereaderlog) << " directory contains " << _sortedPaths.size() << " files" << std::endl;
}

template <typename ImageType>
boost::shared_ptr<ImageBlockReader<ImageType> >
ImageBlockFileFactory<ImageType>::getReader(int n)
{
	LOG_DEBUG(imageblockfilereaderlog) << "For file " << n << " returning path " << _sortedPaths[n].string() << std::endl;
	return boost::make_shared<ImageBlockFileReader<ImageType> >(_sortedPaths[n].string());
}
