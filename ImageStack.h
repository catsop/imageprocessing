#ifndef IMAGEPROCESSING_IMAGE_STACK_H__
#define IMAGEPROCESSING_IMAGE_STACK_H__

#include <pipeline/all.h>
#include "Image.h"
#include "DiscreteVolume.h"

template <typename ImageType>
class ImageStack : public pipeline::Data, public DiscreteVolume {

	// Image objects are shared between ImageStack
	typedef std::vector<boost::shared_ptr<ImageType> > sections_type;

public:

	typedef typename ImageType::value_type         value_type;

	typedef typename sections_type::iterator       iterator;

	typedef typename sections_type::const_iterator const_iterator;

	/**
	 * Remove all sections.
	 */
	void clear();

	/**
	 * Add a single section to this set of sections.
	 */
	void add(boost::shared_ptr<ImageType> section);

	/**
	 * Add a set of sections to this set of sections.
	 */
	void addAll(boost::shared_ptr<ImageStack<ImageType> > stack);

	const const_iterator begin() const { return _sections.begin(); }

	iterator begin() { return _sections.begin(); }

	const const_iterator end() const { return _sections.end(); }

	iterator end() { return _sections.end(); }

	boost::shared_ptr<ImageType> operator[](unsigned int i) { return _sections[i]; }

	boost::shared_ptr<const ImageType> operator[](unsigned int i) const { return _sections[i]; }

	unsigned int size() const { return _sections.size(); }

	unsigned int width() const { return (size() > 0 ? _sections[0]->width() : 0); }

	unsigned int height() const { return (size() > 0 ? _sections[0]->height() : 0); }

protected:

	util::box<unsigned int,3> computeDiscreteBoundingBox() const override {

		return util::box<unsigned int,3>(0, 0, 0, width(), height(), size());
	}

private:

	sections_type _sections;
};

#endif // IMAGEPROCESSING_IMAGE_STACK_H__

