#ifndef IMAGEPROCESSING_MEDIAN_FILTER_H__
#define IMAGEPROCESSING_MEDIAN_FILTER_H__

#include <vigra/functorexpression.hxx>
#include <vigra/flatmorphology.hxx>

#include <pipeline/all.h>
#include "Image.h"

template <typename ImageType>
class MedianFilter : public pipeline::SimpleProcessNode<> {

public:

	MedianFilter();

private:

	void updateOutputs();

	pipeline::Input<int>    _radius;
	pipeline::Input<ImageType>  _image;
	pipeline::Output<ImageType> _filtered;
};

#endif // IMAGEPROCESSING_MEDIAN_FILTER_H__

