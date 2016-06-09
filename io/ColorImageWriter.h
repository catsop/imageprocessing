#ifndef PIPELINE_COLOR_IMAGE_WRITER_H__
#define PIPELINE_COLOR_IMAGE_WRITER_H__

#include <pipeline/all.h>
#include <signals/Slot.h>
#include <imageprocessing/Image.h>

template <typename ImageType>
class ColorImageWriter : public pipeline::SimpleProcessNode<> {

public:

	ColorImageWriter(std::string filename = "");

	/**
	 * Initiate writing of the image that is connected to this writer.
	 */
	void write(std::string filename = "");

private:

	void updateOutputs() {};

	// the input images
	pipeline::Input<ImageType> _r;
	pipeline::Input<ImageType> _g;
	pipeline::Input<ImageType> _b;

	// the name of the file to write to
	std::string _filename;
};

#endif // PIPELINE_COLOR_IMAGE_WRITER_H__


