#ifndef PIPELINE_IMAGE_HTTP_READER_H__
#define PIPELINE_IMAGE_HTTP_READER_H__

#include <pipeline/all.h>
#include <imageprocessing/Image.h>
#include <util/httpclient.h>
#include "ImageReader.h"

struct ImageMissing : virtual IOError {};

template <typename ImageType>
class ImageHttpReader : public ImageReader<ImageType>
{

public:
    ImageHttpReader(std::string url, const HttpClient& client);

protected:
    void readImage();

    using ImageHttpReader::ImageReader::_image;

private:
    std::string _url;

    const HttpClient& _client;
};

#endif //PIPELINE_IMAGE_HTTP_READER_H__
