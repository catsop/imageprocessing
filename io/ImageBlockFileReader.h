#ifndef IMAGE_BLOCK_FILE_READER_H__
#define IMAGE_BLOCK_FILE_READER_H__

#include <boost/shared_ptr.hpp>
#include <boost/filesystem.hpp>
#include <imageprocessing/io/ImageBlockFactory.h>
#include <imageprocessing/io/ImageBlockReader.h>
#include <imageprocessing/ImageCrop.h>
#include <pipeline/all.h>

#include "ImageFileReader.h"

template <typename ImageType>
class ImageBlockFileReader : public ImageBlockReader<ImageType> {
public:
    ImageBlockFileReader(const std::string filename);

protected:
    void readImage();

    using ImageBlockFileReader::ImageBlockReader::_block;
    using ImageBlockFileReader::ImageBlockReader::_image;
    
private:
    boost::shared_ptr<ImageFileReader<ImageType> > _fileReader;
    boost::shared_ptr<ImageCrop<ImageType> > _imageCrop;
};

template <typename ImageType>
class ImageBlockFileFactory : public ImageBlockFactory<ImageType> {
public:
	ImageBlockFileFactory(const std::string& directory);
	boost::shared_ptr<ImageBlockReader<ImageType> > getReader(int n);
	
private:
	std::vector<boost::filesystem::path> _sortedPaths;
};

#endif //IMAGE_BLOCK_FILE_READER_H__