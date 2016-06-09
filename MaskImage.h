#ifndef IMAGEPROCESSING_MASK_IMAGE_H__
#define IMAGEPROCESSING_MASK_IMAGE_H__

#include <vigra/multi_pointoperators.hxx>

#include <pipeline/all.h>
#include "Image.h"

template <typename ImageType>
class MaskImage : public pipeline::SimpleProcessNode<> {

public:

	MaskImage(float maskValue = 0.0);

private:

	void updateOutputs();

	pipeline::Input<ImageType> _image;
	pipeline::Input<ImageType>  _mask;
	pipeline::Output<ImageType> _masked;

	float _maskValue;
};

#endif // IMAGEPROCESSING_MASK_IMAGE_H__

