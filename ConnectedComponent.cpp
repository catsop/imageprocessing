#ifdef __SSE4_1__
#include <emmintrin.h> // SSE 2
#include <smmintrin.h> // SSE 4.1
#endif // __SSE4_1__

#include <boost/make_shared.hpp>

#include <imageprocessing/exceptions.h>
#include "ConnectedComponent.h"

ConnectedComponent::ConnectedComponent(
		boost::shared_ptr<Image> source,
		double value,
		boost::shared_ptr<pixel_list_type> pixelList,
		pixel_list_type::const_iterator begin,
		pixel_list_type::const_iterator end) :

	_pixels(pixelList),
	_value(value),
	_boundingBox(0, 0, 0, 0),
	_center(0, 0),
	_centerDirty(true),
	_source(source),
	_pixelRange(begin, end),
	_bitmapDirty(true) {

#ifdef __SSE4_1__

	// if there is at least one pixel
	if (begin != end) {

		unsigned int minX = begin->x();
		unsigned int maxX = begin->x();
		unsigned int minY = begin->y();
		unsigned int maxY = begin->y();

		unsigned int*__restrict__ pixels = (unsigned int*)&*begin;
		unsigned int*__restrict__ pixelsEnd = (unsigned int*)&*end;

		// Iterate through pixelList until 16-byte alignment is reached.
		while (((std::uintptr_t) pixels % 16) != 0) {

			unsigned int x = *pixels;
			unsigned int y = *(pixels + 1);

			if (x < minX)
				minX = x;
			else if (x > maxX)
				maxX = x;

			if (y < minY)
				minY = y;
			else if (y > maxY)
				maxY = y;

			pixels += 2;
		}

		// Prepare aligned, packed integer values.
		__attribute__((aligned(16))) unsigned int minReadin[4] = {minX, minY, minX, minY};
		__attribute__((aligned(16))) unsigned int maxReadin[4] = {maxX, maxY, maxX, maxY};

		// Guaranteed to have at least 8 XMM registers, so use 4 for cumulative
		// values and 2 for vector values. (Using 8+4 of 16 registers on 64-bit
		// arch yields no performance improvement.)
		__m128i mins1 = _mm_load_si128((__m128i*)minReadin);
		__m128i maxs1 = _mm_load_si128((__m128i*)maxReadin);
		__m128i mins2 = mins1;
		__m128i maxs2 = maxs1;

		// Vectorized loop. Strides two packed integer vectors, each containing
		// both X and Y for two pixels.
		while (pixels < pixelsEnd - 8) {

			__m128i pixelPair1 = _mm_load_si128((__m128i*)pixels);
			__m128i pixelPair2 = _mm_load_si128((__m128i*)(pixels + 4));
			pixels += 8; // Hint compiler to iterate while loads stall.
			_mm_prefetch(pixels, _MM_HINT_T0);
			mins1 = _mm_min_epu32(mins1, pixelPair1);
			maxs1 = _mm_max_epu32(maxs1, pixelPair1);
			mins2 = _mm_min_epu32(mins2, pixelPair2);
			maxs2 = _mm_max_epu32(maxs2, pixelPair2);
		}

		// Combine stride results.
		mins1 = _mm_min_epu32(mins1, mins2);
		maxs1 = _mm_max_epu32(maxs1, maxs2);

		// Iterate through any remaining pixels.
		while (pixels < pixelsEnd) {

			unsigned int x = *pixels;
			unsigned int y = *(pixels + 1);

			if (x < minX)
				minX = x;
			else if (x > maxX)
				maxX = x;

			if (y < minY)
				minY = y;
			else if (y > maxY)
				maxY = y;

			pixels += 2;
		}

		// Readout packed vectors, compare with remaining results, and store.
		__attribute__((aligned(16))) unsigned int minReadout[4];
		__attribute__((aligned(16))) unsigned int maxReadout[4];
		_mm_store_si128((__m128i*)minReadout, mins1);
		_mm_store_si128((__m128i*)maxReadout, maxs1);

		_boundingBox.min().x() = (int)std::min(minX, std::min(minReadout[0], minReadout[2]));
		_boundingBox.max().x() = (int)std::max(maxX, std::max(maxReadout[0], maxReadout[2])) + 1;
		_boundingBox.min().y() = (int)std::min(minY, std::min(minReadout[1], minReadout[3]));
		_boundingBox.max().y() = (int)std::max(maxY, std::max(maxReadout[1], maxReadout[3])) + 1;
	}

#else // __SSE4_1__

	// if there is at least one pixel
	if (begin != end) {

		_boundingBox.min().x() = begin->x();
		_boundingBox.max().x() = begin->x() + 1;
		_boundingBox.min().y() = begin->y();
		_boundingBox.max().y() = begin->y() + 1;
	}

	for (const util::point<unsigned int, 2>& pixel : getPixels()) {

		_boundingBox.min().x() = std::min(_boundingBox.min().x(), (int)pixel.x());
		_boundingBox.max().x() = std::max(_boundingBox.max().x(), (int)pixel.x() + 1);
		_boundingBox.min().y() = std::min(_boundingBox.min().y(), (int)pixel.y());
		_boundingBox.max().y() = std::max(_boundingBox.max().y(), (int)pixel.y() + 1);
	}

#endif // __SSE4_1__
}

double
ConnectedComponent::getValue() const {

	return _value;
}

const util::point<double,2>&
ConnectedComponent::getCenter() const {

	if (_centerDirty) {
		_center.x() = 0;
		_center.y() = 0;

		for (const util::point<unsigned int, 2>& pixel : getPixels()) {

			_center += pixel;
		}

		_center /= getSize();
		_centerDirty = false;
	}

	return _center;
}

const ConnectedComponent::PixelRange&
ConnectedComponent::getPixels() const {

	return _pixelRange;
}

const boost::shared_ptr<ConnectedComponent::pixel_list_type>
ConnectedComponent::getPixelList() const {

	return _pixels;
}

unsigned int
ConnectedComponent::getSize() const {

	return _pixelRange.end() - _pixelRange.begin();
}

const util::box<int,2>&
ConnectedComponent::getBoundingBox() const {

	return _boundingBox;
}

const ConnectedComponent::bitmap_type&
ConnectedComponent::getBitmap() const {

	if (_bitmapDirty) {

		_bitmap.reshape(bitmap_type::size_type(_boundingBox.width(), _boundingBox.height()), false);

		for (const util::point<int, 2>& pixel : getPixels())
			_bitmap(pixel.x() - _boundingBox.min().x(), pixel.y() - _boundingBox.min().y()) = true;

		_bitmapDirty = false;
	}

	return _bitmap;
}

bool
ConnectedComponent::operator<(const ConnectedComponent& other) const {

	return getSize() < other.getSize();
}

ConnectedComponent
ConnectedComponent::translate(const util::point<int,2>& pt)
{
	boost::shared_ptr<pixel_list_type> translation = boost::make_shared<pixel_list_type>(getSize());
	
	for (const util::point<unsigned int, 2>& pixel : getPixels())
	{
		translation->add(pixel + pt);
	}
	
	return ConnectedComponent(_source, _value, translation, translation->begin(), translation->end());
}


ConnectedComponent
ConnectedComponent::intersect(const ConnectedComponent& other) {

	// create a pixel list for the intersection
	boost::shared_ptr<pixel_list_type> intersection;

	// find the intersection pixels
	std::vector<util::point<unsigned int,2> > intersectionPixels;
	bitmap_type::size_type size = getBitmap().shape();
	for(const util::point<unsigned int, 2>& pixel : other.getPixels())
		if (_boundingBox.contains(pixel)) {

			unsigned int x = pixel.x() - _boundingBox.min().x();
			unsigned int y = pixel.y() - _boundingBox.min().y();

			if (x >= size[0] || y >= size[1])
				continue;

			if (_bitmap(x, y))
				intersection->add(pixel);
		}

	return ConnectedComponent(_source, _value, intersection, intersection->begin(), intersection->end());
}

bool ConnectedComponent::intersects(const ConnectedComponent& other)
{
	if (_boundingBox.intersects(other.getBoundingBox()))
	{
		bitmap_type::size_type size = getBitmap().shape();

		for (const util::point<unsigned int, 2>& pixel : other.getPixels())
		{
			if (_boundingBox.contains(pixel)) {

				unsigned int x = pixel.x() - _boundingBox.min().x();
				unsigned int y = pixel.y() - _boundingBox.min().y();

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
	util::box<int,2> thisBound = getBoundingBox();
	util::box<int,2> otherBound = other.getBoundingBox();

	if (thisBound == otherBound && hashValue() == other.hashValue())
	{
		// If this bound equals that bound
		bitmap_type thisBitmap = getBitmap();
		bitmap_type otherBitmap = other.getBitmap();
		
		//Check that the other's bitmap contains all of our pixels.
		for (const util::point<unsigned int, 2>& pixel : getPixels())
		{
			if (!otherBitmap(pixel.x() - thisBound.min().x(), pixel.y() - thisBound.min().y()))
			{
				return false;
			}
		}
		
		//Check that our bitmap contains all of the other's pixels.
		for (const util::point<unsigned int, 2>& pixel : other.getPixels())
		{
			if (!thisBitmap(pixel.x() - otherBound.min().x(), pixel.y() - otherBound.min().y()))
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

