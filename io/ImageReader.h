#ifndef PIPELINE_IMAGE_READER_H__
#define PIPELINE_IMAGE_READER_H__

#include <pipeline/all.h>
#include <imageprocessing/Image.h>

class ImageReader : public pipeline::SimpleProcessNode<> {

public:

	ImageReader(std::string filename);
	
protected:
		/**
	 * Reads the image.
	 */
	void readImage();

	// the output image
	pipeline::Output<Image> _image;
	

private:

	void updateOutputs();

	// the name of the file to read
	std::string _filename;

	// the image data
	vigra::MultiArray<2, float> _imageData;
};

#endif // PIPELINE_IMAGE_READER_H__

