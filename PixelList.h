#ifndef IMAGEPROCESSING_PIXEL_LIST_H__
#define IMAGEPROCESSING_PIXEL_LIST_H__

#include <util/point.hpp>

/**
 * A list of pixel locations. As long as the initially set size is not exceeded, 
 * adding pixels and clearing does not invalidate iterators into the list.
 */
class PixelList {

	typedef std::vector<util::point<unsigned int,2> > pixel_list_type;

public:

	typedef pixel_list_type::iterator       iterator;
	typedef pixel_list_type::const_iterator const_iterator;

	PixelList() {}

	/**
	 * Create a new pixel list of the given size.
	 */
	PixelList(size_t size) {

		_pixelList.reserve(size);
	}

	/**
	 * Add a pixel to the pixel list. Existing iterators are not invalidated, as 
	 * long as 'size' is not exceeded.
	 */
	void add(const util::point<unsigned int,2>& pixel) {

		_pixelList.push_back(pixel);
	}

	/**
	 * Iterator access.
	 */
	iterator       begin() { return _pixelList.begin(); }
	const_iterator begin() const { return _pixelList.begin(); }
	iterator       end() { return _pixelList.end(); }
	const_iterator end() const { return _pixelList.end(); }

	/**
	 * The number of pixels that have been added to this pixel list.
	 */
	size_t size() const { return _pixelList.size(); }

private:

	// a non-resizing vector of pixel locations
	pixel_list_type _pixelList;
};

#endif // IMAGEPROCESSING_PIXEL_LIST_H__

