#ifdef __SSE4_1__
#include <emmintrin.h> // SSE 2
#include <smmintrin.h> // SSE 4.1
#endif // __SSE4_1__

#include <boost/make_shared.hpp>
#include <vigra/distancetransform.hxx>

#include <imageprocessing/exceptions.h>
#include <util/geometry.hpp>
#include "ConnectedComponent.h"

ConnectedComponent::ConnectedComponent(
		std::array<char, 8> value,
		boost::shared_ptr<pixel_list_type> pixelList,
		pixel_list_type::const_iterator begin,
		pixel_list_type::const_iterator end) :

	_pixels(pixelList),
	_value(value),
	_boundingBox(0, 0, 0, 0),
	_center(0, 0),
	_centerDirty(true),
	_pixelRange(begin, end),
	_bitmapDirty(true) {

#ifdef __SSE4_1__

	// if there is at least one pixel
	if (begin != end) {

		unsigned int*__restrict__ pixels    = (unsigned int*)&*begin;
		unsigned int*__restrict__ pixelsEnd = (unsigned int*)&*end;

		// Prepare aligned, packed integer values.
		typedef union {
			__m128i v;
			unsigned int a[4];
		} xmm_uints;

		enum {X1, Y1, X2, Y2};

		__attribute__((aligned(16))) xmm_uints mins1;
		__attribute__((aligned(16))) xmm_uints maxs1;

		mins1.a[X1] = begin->x();
		maxs1.a[X1] = begin->x();
		mins1.a[Y1] = begin->y();
		maxs1.a[Y1] = begin->y();

		// Iterate through pixelList until 16-byte alignment is reached.
		while (((std::uintptr_t) pixels % 16) != 0 && pixels < pixelsEnd) {

			unsigned int x = pixels[X1];
			unsigned int y = pixels[Y1];

			mins1.a[X1] = std::min(mins1.a[X1], x);
			mins1.a[Y1] = std::min(mins1.a[Y1], y);
			maxs1.a[X1] = std::max(maxs1.a[X1], x);
			maxs1.a[Y1] = std::max(maxs1.a[Y1], y);

			pixels += 2;
		}

		// Guaranteed to have at least 8 XMM registers, so use 4 for cumulative
		// values and 2 for vector values. (Using 8+4 of 16 registers on 64-bit
		// arch yields no performance improvement.)
		mins1.a[X2] = mins1.a[X1];
		mins1.a[Y2] = mins1.a[Y1];
		maxs1.a[X2] = maxs1.a[X1];
		maxs1.a[Y2] = maxs1.a[Y1];
		__m128i mins2 = mins1.v;
		__m128i maxs2 = maxs1.v;

		// Vectorized loop. Strides two packed integer vectors, each containing
		// both X and Y for two pixels.
		while (pixels < pixelsEnd - 8) {

			__m128i pixelPair1 = _mm_load_si128((__m128i*)pixels);
			__m128i pixelPair2 = _mm_load_si128((__m128i*)(pixels + 4));
			pixels += 8; // Hint compiler to iterate while loads stall.
			_mm_prefetch(pixels, _MM_HINT_T0);
			mins1.v = _mm_min_epu32(mins1.v, pixelPair1);
			maxs1.v = _mm_max_epu32(maxs1.v, pixelPair1);
			mins2   = _mm_min_epu32(mins2,   pixelPair2);
			maxs2   = _mm_max_epu32(maxs2,   pixelPair2);
		}

		// Combine stride results.
		mins1.v = _mm_min_epu32(mins1.v, mins2);
		maxs1.v = _mm_max_epu32(maxs1.v, maxs2);

		// Iterate through any remaining pixels.
		while (pixels < pixelsEnd) {

			unsigned int x = pixels[X1];
			unsigned int y = pixels[Y1];

			mins1.a[X1] = std::min(mins1.a[X1], x);
			mins1.a[Y1] = std::min(mins1.a[Y1], y);
			maxs1.a[X1] = std::max(maxs1.a[X1], x);
			maxs1.a[Y1] = std::max(maxs1.a[Y1], y);

			pixels += 2;
		}

		// Readout packed vectors, compare with remaining results, and store.
		_boundingBox.min().x() = (int)std::min(mins1.a[X1], mins1.a[X2]);
		_boundingBox.min().y() = (int)std::min(mins1.a[Y1], mins1.a[Y2]);
		_boundingBox.max().x() = (int)std::max(maxs1.a[X1], maxs1.a[X2]) + 1;
		_boundingBox.max().y() = (int)std::max(maxs1.a[Y1], maxs1.a[Y2]) + 1;
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

ConnectedComponent::ConnectedComponent(
		std::array<char, 8> value,
		const util::point<int,2>& offset,
		const bitmap_type& bitmap,
		const size_t size) :

	_pixels(boost::make_shared<pixel_list_type>(size)),
	_value(value),
	_boundingBox(offset.x(), offset.y(), offset.x() + bitmap.width(), offset.y() + bitmap.height()),
	_center(0, 0),
	_centerDirty(true),
	_pixelRange(_pixels->begin(), _pixels->end()),
	_bitmap(bitmap),
	_bitmapDirty(false) {

	for (unsigned int x = 0; x < static_cast<unsigned int>(bitmap.width()); x++)
		for (unsigned int y = 0; y < static_cast<unsigned int>(bitmap.height()); y++)
			if (bitmap(x, y))
				_pixels->add(util::point<unsigned int, 2>(offset.x() + x, offset.y() + y));

	_pixelRange = PixelRange(_pixels->begin(), _pixels->end());
}

std::array<char, 8>
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

const util::point<int, 2>
ConnectedComponent::getInteriorPoint(float centroidBias) const {

	const bitmap_type& thisBitmap = getBitmap();
	const util::point<float, 2> centroid = getCenter() - _boundingBox.min();
	vigra::MultiArray<2, float> interiorDistance(thisBitmap.shape());

	vigra::distanceTransform(thisBitmap, interiorDistance, true, 2);
	float bestDist = -std::numeric_limits<float>::infinity();
	util::point<unsigned int, 2> bestPoint;

	for (unsigned int x = 0; x < thisBitmap.width(); x++)
		for (unsigned int y = 0; y < thisBitmap.height(); y++)
			if (interiorDistance(x, y)) {
				util::point<unsigned int, 2> pixel(x, y);
				// Find a pixel optimizing between the centroid and medial axis.
				// The linear combination is an arbitrary objective.
				float score = interiorDistance(x, y) - centroidBias * util::length(pixel - centroid);
				if (score > bestDist) {
					bestDist = score;
					bestPoint = pixel;
				}
			}

	return bestPoint + _boundingBox.min();
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
	
	return ConnectedComponent(_value, translation, translation->begin(), translation->end());
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

	return ConnectedComponent(_value, intersection, intersection->begin(), intersection->end());
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

