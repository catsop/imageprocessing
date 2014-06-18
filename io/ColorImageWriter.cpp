#include <vigra/impex.hxx>
#include <vigra/rgbvalue.hxx>

#include <util/Logger.h>
#include "ColorImageWriter.h"

logger::LogChannel colorimagewriterlog("colorimagewriterlog", "[ColorImageWriter] ");

ColorImageWriter::ColorImageWriter(std::string filename) :
	_filename(filename) {

	// let others know about our inputs
	registerInput(_r, "r");
	registerInput(_g, "g");
	registerInput(_b, "b");
}

void
ColorImageWriter::write(std::string filename) {

	LOG_DEBUG(colorimagewriterlog) << "requesting image update" << std::endl;

	updateInputs();

	if (!_r.isSet() || !_g.isSet() || !_b.isSet()) {

		LOG_ERROR(colorimagewriterlog) << "no input image set" << std::endl;
		return;
	}

	if (filename == "")
		filename = _filename;

	LOG_DEBUG(colorimagewriterlog) << "attempting to write image" << std::endl;

	if (_r->shape() != _g->shape() || _r->shape() != _b->shape()) {

		LOG_ERROR(colorimagewriterlog) << "images are not of same size" << std::endl;
		return;
	}

	vigra::MultiArray<2, vigra::RGBValue<float> > rgbImage(_r->shape());

	Image::iterator ri = _r->begin();
	Image::iterator gi = _g->begin();
	Image::iterator bi = _b->begin();
	vigra::MultiArray<2, vigra::RGBValue<float> >::iterator rgb = rgbImage.begin();

	while (rgb != rgbImage.end()) {

		rgb->red()   = *ri;
		rgb->green() = *gi;
		rgb->blue()  = *bi;

		ri++;
		gi++;
		bi++;
		rgb++;
	}

	// save to file
	vigra::exportImage(vigra::srcImageRange(rgbImage), vigra::ImageExportInfo(filename.c_str()));

	LOG_DEBUG(colorimagewriterlog) << "image written" << std::endl;
}

