#ifndef PIPELINE_IMAGE_WRITER_H__
#define PIPELINE_IMAGE_WRITER_H__

#include <pipeline/all.h>
#include <signals/Slot.h>
#include <util/endian.h>
#include <imageprocessing/Image.h>


// PNG is big endian and will handle the byte order of individual shorts
// packed into RGBA channels, but we are still responsible for the order
// the shorts are packed into the channels from the 64-bit label so that the
// entire RGBA value is big endian.
#if __BYTE_ORDER == __LITTLE_ENDIAN
	const size_t LABEL_IMAGE_RGBA_ORDER[] = {3, 2, 1, 0}; // R, G, B, A
#else
	const size_t LABEL_IMAGE_RGBA_ORDER[] = {0, 1, 2, 3};
#endif


template <typename ImageType>
class ImageWriter : public pipeline::SimpleProcessNode<> {

public:

	ImageWriter(std::string filename = "");

	/**
	 * Initiate writing of the image that is connected to this writer.
	 */
	void write(std::string filename = "");

private:

	void writeToFile(std::string filename = "");

	void updateOutputs() {};

	// the input image
	pipeline::Input<ImageType> _image;

	// the name of the file to write to
	std::string _filename;
};

#endif // PIPELINE_IMAGE_WRITER_H__

