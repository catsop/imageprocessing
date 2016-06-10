#include <limits>

#include <Magick++.h>

#include <util/exceptions.h>
#include <util/Logger.h>
#include "ImageHttpReader.h"
#include "ImageWriter.h"

logger::LogChannel imagehttpreaderlog("imagehttpreaderlog", "[ImageHttpReader] ");

template <typename ImageType>
ImageHttpReader<ImageType>::ImageHttpReader(std::string url, const HttpClient& client) :
    _url(url),
    _client(client)
{

}

template <typename ImageType>
void
pushPixels(ImageType& image, Magick::PixelPacket* pixels);

template <typename ImageType>
void
ImageHttpReader<ImageType>::readImage()
{
    LOG_DEBUG(imagehttpreaderlog) << "Will attempt to read image from " << _url << std::endl;

    //Read the image url
    HttpClient::response res = _client.get(_url);
    if (res.code != 200 /* http */ && res.code != 0 /* file */)
    {
        if (res.code == 404)
        {
            // Handle missing images separately from general HTTP errors which
            // may be transient.
            UTIL_THROW_EXCEPTION(
                    ImageMissing,
                    "Image not found (404) " << _url);
        }
        else
        {
            UTIL_THROW_EXCEPTION(
                    IOError,
                    "While attempting to GET image " << _url <<
                    " received response status " << res.code <<
                    " with body " << res.body);
        }
    }
    int size = res.body.size();

    LOG_DEBUG(imagehttpreaderlog) << "Read " << size << " things" << std::endl;

    // Create a blob from the body as an array of char.
    // I'm frankly surprised that this works.
    Magick::Blob blob(res.body.c_str(), size);
    // Make the image.
    Magick::Image image(blob);
    // Define the pixel packet reference.
    Magick::PixelPacket *pixels;
    // Get the size of the image.
    int w = image.columns(), h = image.rows();

	// allocate output image
	_image = new ImageType(w, h);

    LOG_DEBUG(imagehttpreaderlog) << "Image is size " << w << " by " << h << std::endl;

    // Get a pixel reference
    pixels = image.getPixels(0, 0, w, h);

    LOG_DEBUG(imagehttpreaderlog) << "Pushing pixels..." << std::endl;

    pushPixels<ImageType>(*_image, pixels);
}

template <>
void
pushPixels<IntensityImage>(IntensityImage& image, Magick::PixelPacket* pixels) {

    for (int i = 0; i < image.size(); ++i) {

        double dval = Magick::Color::scaleQuantumToDouble(pixels[i].green);
        image[i] = static_cast<IntensityImage::value_type>(dval);
    }
}

template <>
void
pushPixels<LabelImage>(LabelImage& image, Magick::PixelPacket* pixels) {

    for (int i = 0; i < image.size(); ++i) {

        LabelImage::value_type label = 0;
        unsigned short *label_shorts = reinterpret_cast<unsigned short *>(&label);
        label_shorts[LABEL_IMAGE_RGBA_ORDER[0]] = pixels[i].red;
        label_shorts[LABEL_IMAGE_RGBA_ORDER[1]] = pixels[i].green;
        label_shorts[LABEL_IMAGE_RGBA_ORDER[2]] = pixels[i].blue;
        label_shorts[LABEL_IMAGE_RGBA_ORDER[3]] = std::numeric_limits<unsigned short>::max() - pixels[i].opacity;
        image[i] = label;
    }
}

EXPLICITLY_INSTANTIATE_COMMON_IMAGE_TYPES(ImageHttpReader);
