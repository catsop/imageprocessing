#include <vigra/functorexpression.hxx>

#include "MaskImage.h"

template <typename ImageType>
MaskImage<ImageType>::MaskImage(float maskValue) :
		_masked(new ImageType()),
		_maskValue(maskValue) {

	registerInput(_image, "image");
	registerInput(_mask, "mask");
	registerOutput(_masked, "masked");
}

template <typename ImageType>
void
MaskImage<ImageType>::updateOutputs() {

	int width  = _image->width();
	int height = _image->height();

	_masked->reshape(width, height);

	vigra::combineTwoMultiArrays(
			srcMultiArrayRange(*_image),
			srcMultiArray(*_mask),
			destMultiArray(*_masked),
			ifThenElse(vigra::functor::Arg2() == vigra::functor::Param(1), vigra::functor::Arg1(), vigra::functor::Param(_maskValue)));
}
