#ifndef IMAGEPROCESSING_CONNECTED_COMPONENT_H__
#define IMAGEPROCESSING_CONNECTED_COMPONENT_H__

#include <boost/shared_ptr.hpp>

#include <imageprocessing/Image.h>
#include <imageprocessing/PixelList.h>
#include <util/point.hpp>
#include <util/box.hpp>
#include <util/Logger.h>
#include <util/Hashable.h>
#include <pipeline/all.h>
#include "ConnectedComponentHash.h"

class ConnectedComponent : public pipeline::Data, public Hashable<ConnectedComponent, ConnectedComponentHash> {

public:

	typedef PixelList                               pixel_list_type;
	typedef vigra::MultiArray<2, bool>              bitmap_type;
	typedef pixel_list_type::iterator               iterator;
	typedef pixel_list_type::const_iterator         const_iterator;

	class PixelRange {

	public:

		PixelRange(
				const_iterator begin,
				const_iterator end) :
			_begin(begin),
			_end(end) {}

		const_iterator begin() const { return _begin; }
		const_iterator end()   const { return _end;   }

	private:

		const_iterator _begin;
		const_iterator _end;
	};

	ConnectedComponent(
			double value,
			boost::shared_ptr<pixel_list_type> pixelList,
			pixel_list_type::const_iterator begin,
			pixel_list_type::const_iterator end);

	ConnectedComponent(
			double value,
			const util::point<int,2>& offset,
			const bitmap_type& bitmap,
			const size_t size);

	/**
	 * Get the intensity value that was assigned to this component.
	 */
	double getValue() const;

	/**
	 * Get a begin and end iterator to the pixels that belong to this component.
	 */
	const PixelRange& getPixels() const;

	/**
	 * Get the pixel list this component is using.
	 */
	const boost::shared_ptr<pixel_list_type> getPixelList() const;

	/**
	 * Get the number of pixels of this component.
	 */
	unsigned int getSize() const;

	/**
	 * Get the mean pixel location of this component.
	 */
	const util::point<double,2>& getCenter() const;

	/**
	 * Get a pixel location near the interior and centroid of this component.
	 */
	const util::point<int,2> getInteriorPoint(float centroidBias = 0.5) const;

	/**
	 * Get the bounding box of this component.
	 */
	const util::box<int,2>& getBoundingBox() const;

	/**
	 * Get a bitmap of the size of the bounding box with values 'true' for every
	 * pixel that belongs to this component.
	 */
	const bitmap_type& getBitmap() const;

	/**
	 * Compare the sizes of two connected components.
	 *
	 * @param other Another component.
	 * @return true, if the other component is bigger.
	 */
	bool operator<(const ConnectedComponent& other) const;

	/**
	 * Create a ConnectedComponent that is the translation of this one.
	 * @param pt the point representing the translation vector.
	 * @return The translation of this ConnectedComponent by pt.
	 */
	ConnectedComponent translate(const util::point<int,2>& pt);
	
	/**
	 * Intersect this connected component with another one.
	 *
	 * @param other The component to intersect with.
	 * @return The intersection of this and another component.
	 */
	ConnectedComponent intersect(const ConnectedComponent& other);

	/**
	 * Check if two connected components intersect.
	 */
	bool intersects(const ConnectedComponent& other);

	/**
	 * Test equality of this ConnectedComponent against another by geometry.
	 */
	bool operator==(const ConnectedComponent& other) const;

private:

	// a list of pixel locations that belong to this component (can be shared
	// between the connected components)
	boost::shared_ptr<pixel_list_type> _pixels;

	// the threshold, at which this connected component was found
	double                                  _value;

	// the min and max x and y values
	util::box<int,2>                        _boundingBox;

	// the center of mass of this component
	mutable util::point<double, 2>          _center;

	mutable bool _centerDirty;

	// the range of the pixels in _pixels that belong to this component (can
	// be all of them, if the pixel lists are not shared)
	PixelRange _pixelRange;

	// a binary map of the size of the bounding box to indicate which pixels
	// belong to this component
	mutable bitmap_type _bitmap;

	mutable bool _bitmapDirty;
};

#endif // IMAGEPROCESSING_CONNECTED_COMPONENT_H__

