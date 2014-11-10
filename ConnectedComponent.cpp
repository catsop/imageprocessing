#include <boost/make_shared.hpp>

#include <boost/functional/hash.hpp>
#include <imageprocessing/exceptions.h>
#include "ConnectedComponent.h"

ConnectedComponent::ConnectedComponent() :
	_boundingBox(0, 0, 0, 0),
	_center(0, 0) {}

ConnectedComponent::ConnectedComponent(
		boost::shared_ptr<Image> source,
		double value,
		boost::shared_ptr<pixel_list_type> pixelList,
		unsigned int begin,
		unsigned int end) :

	_pixels(pixelList),
	_value(value),
	_boundingBox(0, 0, 0, 0),
	_center(0, 0),
	_centerDirty(true),
	_source(source),
	_begin(_pixels->begin() + begin),
	_end(_pixels->begin() + end),
	_bitmapDirty(true),
	_hashValue(0),
	_hashDirty(true) {

	// if there is at least one pixel
	if (begin != end) {

		_boundingBox.minX = _begin->x;
		_boundingBox.maxX = _begin->x + 1;
		_boundingBox.minY = _begin->y;
		_boundingBox.maxY = _begin->y + 1;
	}

	foreach (const util::point<unsigned int>& pixel, getPixels()) {

		_boundingBox.minX = std::min(_boundingBox.minX, (int)pixel.x);
		_boundingBox.maxX = std::max(_boundingBox.maxX, (int)pixel.x + 1);
		_boundingBox.minY = std::min(_boundingBox.minY, (int)pixel.y);
		_boundingBox.maxY = std::max(_boundingBox.maxY, (int)pixel.y + 1);
	}
}

double
ConnectedComponent::getValue() const {

	return _value;
}

const util::point<double>&
ConnectedComponent::getCenter() const {

	if (_centerDirty) {
		_center.x = 0;
		_center.y = 0;

		foreach (const util::point<unsigned int>& pixel, getPixels()) {

			_center += pixel;
		}

		_center /= getSize();
		_centerDirty = false;
	}

	return _center;
}

const std::pair<ConnectedComponent::const_iterator, ConnectedComponent::const_iterator>
ConnectedComponent::getPixels() const {

	return std::make_pair(_begin, _end);
}

const boost::shared_ptr<ConnectedComponent::pixel_list_type>
ConnectedComponent::getPixelList() const {

	return _pixels;
}

unsigned int
ConnectedComponent::getSize() const {

	return _end - _begin;
}

const util::rect<int>&
ConnectedComponent::getBoundingBox() const {

	return _boundingBox;
}

const ConnectedComponent::bitmap_type&
ConnectedComponent::getBitmap() const {

	if (_bitmapDirty) {

		_bitmap.reshape(bitmap_type::size_type(_boundingBox.width(), _boundingBox.height()), false);

		foreach (const util::point<int>& pixel, getPixels())
			_bitmap(pixel.x - _boundingBox.minX, pixel.y - _boundingBox.minY) = true;

		_bitmapDirty = false;
	}

	return _bitmap;
}

bool
ConnectedComponent::operator<(const ConnectedComponent& other) const {

	return getSize() < other.getSize();
}

ConnectedComponent
ConnectedComponent::translate(const util::point<int>& pt)
{
	boost::shared_ptr<pixel_list_type> translation = boost::make_shared<pixel_list_type>();
	
	foreach (const util::point<unsigned int>& pixel, getPixels())
	{
		translation->push_back(pixel + pt);
	}
	
	return ConnectedComponent(_source, _value, translation, 0, translation->size());
}


ConnectedComponent
ConnectedComponent::intersect(const ConnectedComponent& other) {

	boost::shared_ptr<pixel_list_type> intersection = boost::make_shared<pixel_list_type>();

	bitmap_type::size_type size = getBitmap().shape();

	foreach (const util::point<unsigned int>& pixel, other.getPixels())
		if (_boundingBox.contains(pixel)) {

			unsigned int x = pixel.x - _boundingBox.minX;
			unsigned int y = pixel.y - _boundingBox.minY;

			if (x >= size[0] || y >= size[1])
				continue;

			if (_bitmap(x, y))
				intersection->push_back(pixel);
		}

	return ConnectedComponent(_source, _value, intersection, 0, intersection->size());
}

bool ConnectedComponent::intersects(const ConnectedComponent& other)
{
	if (_boundingBox.intersects(other.getBoundingBox()))
	{
		bitmap_type::size_type size = getBitmap().shape();

		foreach (const util::point<unsigned int>& pixel, other.getPixels())
		{
			if (_boundingBox.contains(pixel)) {

				unsigned int x = pixel.x - _boundingBox.minX;
				unsigned int y = pixel.y - _boundingBox.minY;

				if (x >= size[0] || y >= size[1])
					continue;

				if (_bitmap(x, y))
				{
					return true;
				}
			}
		}
	}
	
	return false;
}


bool
ConnectedComponent::operator==(const ConnectedComponent& other) const
{
	util::rect<int> thisBound = getBoundingBox();
	util::rect<int> otherBound = other.getBoundingBox();

	if (thisBound == otherBound && hashValue() == other.hashValue())
	{
		// If this bound equals that bound
		bitmap_type thisBitmap = getBitmap();
		bitmap_type otherBitmap = other.getBitmap();
		
		//Check that the other's bitmap contains all of our pixels.
		foreach (const util::point<unsigned int> pixel, getPixels())
		{
			if (!otherBitmap(pixel.x - thisBound.minX, pixel.y - thisBound.minY))
			{
				return false;
			}
		}
		
		//Check that our bitmap contains all of the other's pixels.
		foreach (const util::point<unsigned int> pixel, other.getPixels())
		{
			if (!thisBitmap(pixel.x - otherBound.minX, pixel.y - otherBound.minY))
			{
				return false;
			}
		}
		
		//If both conditions are true, both components contain each other, and are therefore equal.
		return true;
	}
	else
	{
		// If our bound is unequal to the other's bound, then we're unequal.
		return false;
	}
}

std::size_t
ConnectedComponent::hashValue() const
{
	if (_hashDirty) {
		// Calculate geometric hash value
		boost::hash_combine(_hashValue, boost::hash_value(_boundingBox.minX));
		boost::hash_combine(_hashValue, boost::hash_value(_boundingBox.minY));
		boost::hash_combine(_hashValue, boost::hash_value(_boundingBox.maxX));
		boost::hash_combine(_hashValue, boost::hash_value(_boundingBox.maxY));

		const bitmap_type thisBitmap = getBitmap();
		for (int x = 0; x < thisBitmap.width(); ++x)
		{
			for (int y = 0; y < thisBitmap.height(); ++y)
			{
				if (thisBitmap(x, y))
				{
					boost::hash_combine(_hashValue, boost::hash_value(x + _boundingBox.minX));
					boost::hash_combine(_hashValue, boost::hash_value(y + _boundingBox.minY));
				}
			}
		}

		_hashDirty = false;
	}

	return _hashValue;
}
