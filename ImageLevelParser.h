#ifndef IMAGEPROCESSING_IMAGE_LEVEL_PARSER_H__
#define IMAGEPROCESSING_IMAGE_LEVEL_PARSER_H__

#include <stack>
#include <type_traits>
#include <limits>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/container/flat_map.hpp>

#include <vigra/transformimage.hxx>
#include <vigra/functorexpression.hxx>

#include <util/Logger.h>
#include "PixelList.h"
#include "Image.h"

extern logger::LogChannel imagelevelparserlog;

namespace image_level_parser_detail {

	template <typename Precision>
	class BoundaryLocations {

	public:

		typedef util::point<unsigned int,2> point_type;

		/**
		 * Put a boundary location with its level on the stack of open boundary
		 * locations.
		 */
		virtual void push(
				const point_type& location,
				const Precision   level) = 0;

		/**
		 * Get the next open boundary location with the given level. Returns
		 * false if there is none.
		 *
		 * @param level
		 *              The reference level.
		 *
		 * @param boundaryLocation
		 *              [out] The next boundary location with level smaller then
		 *              level.
		 */
		virtual bool pop(
				const Precision level,
				point_type&     boundaryLocation) = 0;

		/**
		 * Get the lowest open boundary location that has a level smaller than
		 * the given level.
		 *
		 * @param level
		 *              The reference level.
		 *
		 * @param boundaryLocation
		 *              [out] The lowest boundary location with level smaller
		 *              then level.
		 *
		 * @param boundaryLevel
		 *              [out] The level of the found boundary location.
		 *
		 * @return true, if there is such a boundary location
		 */
		virtual bool popLowest(
				const Precision level,
				point_type&     boundaryLocation,
				Precision&      boundaryLevel) = 0;

		/**
		 * Get the next open boundary location that has a level higher than the
		 * given level.
		 *
		 * @param level
		 *              The reference level.
		 *
		 * @param boundaryLocation
		 *              [out] The next boundary location with level higher then
		 *              level.
		 *
		 * @param boundaryLevel
		 *              [out] The level of the found boundary location.
		 *
		 * @return true, if there is such a boundary location
		 */
		virtual bool popHigher(
				const Precision level,
				point_type&     boundaryLocation,
				Precision&      boundaryLevel) = 0;
	};

	template <typename Precision>
	class DenseBoundaryLocations : BoundaryLocations<Precision> {

	public:

		typedef typename BoundaryLocations<Precision>::point_type point_type;

		DenseBoundaryLocations(Precision maxValue) :
			_boundaryLocations(maxValue + 1),
			MAX_LEVEL(maxValue) {}

		void push(
				const point_type& location,
				const Precision   level);

		bool pop(
				const Precision level,
				point_type&     boundaryLocation);

		bool popLowest(
				const Precision level,
				point_type&     boundaryLocation,
				Precision&      boundaryLevel);

		bool popHigher(
				const Precision level,
				point_type&     boundaryLocation,
				Precision&      boundaryLevel);

	private:
		typedef std::vector<std::stack<point_type> > boundary_locations_type;

		boundary_locations_type _boundaryLocations;

		const Precision MAX_LEVEL;
	};

	template <typename Precision>
	class SparseBoundaryLocations : BoundaryLocations<Precision> {

	public:

		typedef typename BoundaryLocations<Precision>::point_type point_type;

		SparseBoundaryLocations(Precision maxValue) {
			_boundaryLocations.reserve(std::min<size_t>({(size_t)maxValue + 1, 1024, 1<<(sizeof(Precision))}));
		}

		void push(
				const point_type& location,
				const Precision   level);

		bool pop(
				const Precision level,
				point_type&     boundaryLocation);

		bool popLowest(
				const Precision level,
				point_type&     boundaryLocation,
				Precision&      boundaryLevel);

		bool popHigher(
				const Precision level,
				point_type&     boundaryLocation,
				Precision&      boundaryLevel);

	private:
		typedef boost::container::flat_map<Precision, std::stack<point_type> > boundary_locations_type;

		bool pop(
			typename boundary_locations_type::value_type& val,
			point_type& boundaryLocation);

		boundary_locations_type _boundaryLocations;
	};

	template <typename Precision>
	struct Traits {
		typedef SparseBoundaryLocations<Precision> boundary_locations_type;
	};

	template <>
	struct Traits<unsigned char> {
		typedef DenseBoundaryLocations<unsigned char> boundary_locations_type;
	};
}

/**
 * Parses the pixels of an image in terms of the connected components of varying 
 * intensity thresholds in linear time. For each connected component and each 
 * threshold value, a user specified callback is invoked.
 *
 * The number of thresholds applied is given by the Precision template argument: 
 * The input image is discretized into the range of Precision, and all possible 
 * thresholds are applied (for example, unsigned char corresponds to 255 
 * thresholds).
 */
template <typename Precision = unsigned char, typename ImageType = IntensityImage>
class ImageLevelParser {

public:

	/**
	 * Parameters of the image level parser.
	 */
	struct Parameters {

		Parameters() : darkToBright(true), minIntensity(0), maxIntensity(0), spacedEdgeImage(false) {}

		// start processing the dark regions
		bool darkToBright;

		/**
		 * The min and max intensity of the image, used for discretization into 
		 * the Precision type. The default is 0 for both, in which case the 
		 * image is inspected to find them. You can set them to avoid this 
		 * inspection or to ensure that the values of the connected components 
		 * math across different images that might have different intensity 
		 * extrema.
		 */
		typename ImageType::value_type minIntensity;
		typename ImageType::value_type maxIntensity;

		/**
		 * Indicate that the image to process is a spaced edge image. A spaced 
		 * edge image is scaled by a factor of 2 in each dimension, and the 
		 * original values of pixel (x,y) are now in (2x,2y). The odd locations 
		 * of the spaced edge image indicate edges, such that a component tree 
		 * can be extracted even if components are touching (i.e., they don't 
		 * need to have a boundary that separates them). Setting this flag 
		 * ensures that the pixels in the pixel list are only from even 
		 * locations (2x, 2y) and are stored as (x,y).
		 */
		bool spacedEdgeImage;
	};

	/**
	 * Base class and interface definition of visitors that are accepted by the 
	 * parse methods. Visitors don't need to inherit from this class (as long as 
	 * they implement the same interface). This class is provided for 
	 * convenience with no-op methods.
	 */
	class Visitor {

	public:

		/**
		 * Invoked whenever a new component is added as a child of the current 
		 * component, starting from the root (the whole image component) in a 
		 * depth first manner.  Indicates that we go down by one level in the 
		 * component tree and make the new child the current component.
		 *
		 * @param value
		 *              The threshold value of the new child.
		 */
		void newChildComponent(typename ImageType::value_type /*value*/) {}

		/**
		 * Set the pixel list that contains the pixel locations of each 
		 * component. The iterators passed by finalizeComponent refer to indices 
		 * in this pixel list.
		 *
		 * @param pixelList
		 *              A pixel list shared between all components.
		 */
		void setPixelList(boost::shared_ptr<PixelList> pixelList) {}

		/**
		 * Invoked whenever the current component was extracted entirely.  
		 * Indicates that we go up by one level in the component tree and make 
		 * the parent of the current component the new current component.
		 *
		 * @param value
		 *              The threshold value of the current component.
		 *
		 * @param begin, end
		 *              Iterators into the pixel list that define the pixels of 
		 *              the current component.
		 */
		void finalizeComponent(
				typename ImageType::value_type value,
				PixelList::const_iterator      begin,
				PixelList::const_iterator      end) {}
	};

	/**
	 * Create a new image level parser for the given image with the given 
	 * parameters.
	 */
	ImageLevelParser(const ImageType& image, const Parameters& parameters = Parameters());

	/**
	 * Parse the image. The provided visitor has to implement the interface of 
	 * Visitor (but does not need to inherit from it).
	 *
	 * This method will be called for every connected component at every 
	 * threshold, where value is the original value (before discretization) of 
	 * the threshold, and begin and end are iterators into pixelList that span 
	 * all pixels of the connected component.
	 *
	 * The visitor can assume that the callback is invoked following a weak 
	 * ordering of the connected component according to the subset relation.
	 */
	template <typename VisitorType>
	void parse(VisitorType& visitor);

private:

	typedef util::point<unsigned int,2> point_type;

	/**
	 * Set the current location and level.
	 */
	template <typename VisitorType>
	void gotoLocation(const point_type& location, VisitorType& visitor);

	/**
	 * Fill the level at the current location. Returns true, if the level is 
	 * bounded (only higher levels surround it); otherwise false.
	 */
	template <typename VisitorType>
	void fillLevel(VisitorType& visitor);

	/**
	 * In the current boundary locations, try to find the lowest level that is 
	 * higher than the current level and go there. If such a level exists, all 
	 * the open components until this level are closed (and the visitor 
	 * informed) and true is returned. Otherwise, all remaining open components 
	 * are closed (including the one for MaxValue) and false is returned.
	 */
	template <typename VisitorType>
	bool gotoHigherLevel(VisitorType& visitor);

	/**
	 * In the current boundary locations, try to find the lowest level that is 
	 * lower than the reference level and go there. If such a level exists, true 
	 * is returned.
	 */
	template <typename VisitorType>
	bool gotoLowerLevel(Precision referenceLevel, VisitorType& visitor);

	/**
	 * Begin a new connected component at the current location for the given 
	 * level.
	 */
	template <typename VisitorType>
	void beginComponent(Precision level, VisitorType& visitor);

	/**
	 * End a connected component at the current location.
	 *
	 * @return The begin and end iterator of the new connected component in the 
	 * pixel list.
	 */
	template <typename VisitorType>
	void endComponent(Precision level, VisitorType& visitor);

	/**
	 * Find the neighbor of the current position in the given direction. Returns 
	 * false, if the neighbor is not valid (out of bounds or already visited).  
	 * Otherwise, neighborLocation and neighborLevel are set and true is 
	 * returned.
	 */
	typedef unsigned char Direction;
	static const Direction Right;
	static const Direction Down;
	static const Direction Left;
	static const Direction Up;
	bool findNeighbor(Direction direction, point_type& neighborLocation, Precision& neighborLevel);

	// TODO:
	// The following methods have specializations for when Precision is the
	// same as ImageType::value_type. These are necessary for, e.g., lossless
	// label retrieval. Unfortunately there does not seem to be any way to
	// achieve specialization with enable_if template parameters SFINAE for
	// methods of a templated class, so of the remaining implementation
	// strategies overloading is used for clarity.

	/**
	 * Discretized the input image into the range defined by Precision.
	 */
	void discretizeImage(const ImageType& image) {
		discretizeImageImpl(image, std::is_same<Precision, typename ImageType::value_type>());
	}
	void discretizeImageImpl(const ImageType& image, std::false_type);
	void discretizeImageImpl(const ImageType& image, std::true_type);

	/**
	 * Get the orignal value that corresponds to the given discretized value.
	 */
	typename ImageType::value_type getOriginalValue(Precision value) {
		return getOriginalValueImpl(value, std::is_same<Precision, typename ImageType::value_type>());
	}
	typename ImageType::value_type getOriginalValueImpl(Precision value, std::false_type);
	typename ImageType::value_type getOriginalValueImpl(Precision value, std::true_type);

	static const Precision MaxValue;

	// discretized version of the input image
	vigra::MultiArray<2, Precision> _image;

	// min and max value of the original image
	typename ImageType::value_type _min, _max;

	// parameters of the parsing algorithm
	Parameters _parameters;

	// the current location of the parsing algorithm
	point_type _currentLocation;
	Precision  _currentLevel;
	bool       _initCurrentLevel; // Indicates initializing the current level, since we cannot express MaxValue + 1

	// the pixel list, shared ownership with visitors
	boost::shared_ptr<PixelList> _pixelList;

	// a seperate pixel list to transparently handle the spacedEdgeImage flag
	boost::shared_ptr<PixelList> _condensedPixelList;

	// stacks of open boundary locations
	typename image_level_parser_detail::Traits<Precision>::boundary_locations_type _boundaryLocations;

	// stack of component begin iterators (with the level they have been 
	// generated for)
	std::stack<std::pair<Precision, PixelList::iterator> > _componentBegins;
	// another one for the condensed pixel list
	std::stack<std::pair<Precision, PixelList::iterator> > _condensedComponentBegins;

	// visited flag for each pixel
	vigra::MultiArray<2, bool> _visited;
};

template <typename Precision, typename ImageType>
const Precision ImageLevelParser<Precision, ImageType>::MaxValue = std::numeric_limits<Precision>::max();
template <typename Precision, typename ImageType>
const typename ImageLevelParser<Precision, ImageType>::Direction ImageLevelParser<Precision, ImageType>::Right = 0;
template <typename Precision, typename ImageType>
const typename ImageLevelParser<Precision, ImageType>::Direction ImageLevelParser<Precision, ImageType>::Down  = 1;
template <typename Precision, typename ImageType>
const typename ImageLevelParser<Precision, ImageType>::Direction ImageLevelParser<Precision, ImageType>::Left  = 2;
template <typename Precision, typename ImageType>
const typename ImageLevelParser<Precision, ImageType>::Direction ImageLevelParser<Precision, ImageType>::Up    = 3;

template <typename Precision, typename ImageType>
ImageLevelParser<Precision, ImageType>::ImageLevelParser(const ImageType& image, const Parameters& parameters) :
	_parameters(parameters),
	_initCurrentLevel(false),
	_pixelList(boost::make_shared<PixelList>(image.size())),
	_boundaryLocations(MaxValue) {

	if (_parameters.spacedEdgeImage)
		_condensedPixelList = boost::make_shared<PixelList>(image.size()/4);

	_visited.reshape(image.shape());
	_visited = false;

	LOG_ALL(imagelevelparserlog) << "initializing for image of size " << image.size() << std::endl;

	this->discretizeImage(image);
}

template <typename Precision, typename ImageType>
template <typename VisitorType>
void
ImageLevelParser<Precision, ImageType>::parse(VisitorType& visitor) {

	LOG_ALL(imagelevelparserlog) << "parsing image" << std::endl;

	if (_parameters.spacedEdgeImage)
		visitor.setPixelList(_condensedPixelList);
	else
		visitor.setPixelList(_pixelList);

	// Pretend we come from level MaxValue + 1...
	_currentLevel = MaxValue;
	_initCurrentLevel = true;

	// ...and go to our initial pixel. This way we make sure enough components 
	// are put on the stack.
	gotoLocation(point_type(0, 0), visitor);

	LOG_ALL(imagelevelparserlog)
			<< "starting at " << _currentLocation
			<< " with level " << (int)_currentLevel
			<< std::endl;

	// loop through the image
	while (true) {

		// fill the current level
		fillLevel(visitor);

		//LOG_ALL(imagelevelparserlog)
				//<< "filled current level"
				//<< std::endl;

		// try go to the smallest higher level, according to our open 
		// boundary list
		if (!gotoHigherLevel(visitor)) {

			//LOG_ALL(imagelevelparserlog)
					//<< "there are no more higher levels -- we are done"
					//<< std::endl;

			// if there are no higher levels, we are done
			return;
		}
	}
}

template <typename Precision, typename ImageType>
template <typename VisitorType>
void
ImageLevelParser<Precision, ImageType>::gotoLocation(const point_type& newLocation, VisitorType& visitor) {

	Precision newLevel = _image(newLocation.x(), newLocation.y());

	// if we descend
	if (_currentLevel > newLevel || _initCurrentLevel) {

		// begin a new component for each level that we descend
		for (Precision level = _currentLevel - (_initCurrentLevel ? 0 : 1);; level--) {

			_initCurrentLevel = false;

			beginComponent(level, visitor);

			if (level == newLevel)
				break;
		}

	// if we ascend
	} else if (_currentLevel < newLevel) {

		// close one component for each level that we ascend
		for (Precision level = _currentLevel;; level++) {

			endComponent(level, visitor);

			if (level == newLevel - 1)
				break;
		}
	}

	// go to the new location
	_currentLocation = newLocation;
	_currentLevel    = newLevel;

	// the first time we are here?
	if (!_visited(newLocation.x(), newLocation.y())) {

		// mark it as visited and add it to the pixel list
		_visited(newLocation.x(), newLocation.y()) = true;

		if (_parameters.spacedEdgeImage)
			if (newLocation.x() % 2 == 0 && newLocation.y() % 2 == 0)
				_condensedPixelList->add(newLocation/2);

		_pixelList->add(newLocation);
	}
}

template <typename Precision, typename ImageType>
template <typename VisitorType>
void
ImageLevelParser<Precision, ImageType>::fillLevel(VisitorType& visitor) {

	// we are supposed to fill all adjacent pixels of the current pixel that 
	// have the same level
	Precision targetLevel = _currentLevel;

	LOG_ALL(imagelevelparserlog) << "filling level " << (int)targetLevel << std::endl;

	point_type neighborLocation;
	Precision  neighborLevel;

	// walk around...
	while (true) {

		//LOG_ALL(imagelevelparserlog) << "I am at " << _currentLocation << 
		//std::endl;

		// look at all valid neighbors
		for (Direction direction = 0; direction < 4; direction++) {

			// is this a valid neighbor?
			if (!findNeighbor(direction, neighborLocation, neighborLevel))
				continue;

			if (neighborLevel < targetLevel) {

				// We found a smaller neighbor. Interrupt filling the current 
				// level and fill the smaller one first.

				//LOG_ALL(imagelevelparserlog)
						//<< "neighbor is smaller (" << (int)neighborLevel
						//<< "), will go down" << std::endl;

				// remember where we are
				point_type currentLocation = _currentLocation;

				// remember the lower neighbor location
				_boundaryLocations.push(neighborLocation, neighborLevel);

				// fill all levels that are lower than our target level (calls 
				// to fillLevel might add more then the one we just found)
				while (gotoLowerLevel(targetLevel, visitor))
					fillLevel(visitor);

				// go back to where we were
				gotoLocation(currentLocation, visitor);

			} else if (neighborLevel > targetLevel) {

				//LOG_ALL(imagelevelparserlog)
						//<< "neighbor is larger (" << (int)neighborLevel
						//<< "), will remember it" << std::endl;

				// we found a larger neighbor -- remember it
				_boundaryLocations.push(neighborLocation, neighborLevel);

			} else {

				//LOG_ALL(imagelevelparserlog)
						//<< "neighbor is equal (" << (int)neighborLevel
						//<< "), will remember it" << std::endl;

				// we found an equal neighbor -- remember it
				_boundaryLocations.push(neighborLocation, neighborLevel);
			}
		}

		// try to find the next non-visited boundary location of the current 
		// level
		while (true) {

			point_type newLocation;
			bool found = _boundaryLocations.pop(targetLevel, newLocation);

			// if there aren't any other boundary locations of the current 
			// level, we are done and bounded
			if (!found) {

				//LOG_ALL(imagelevelparserlog)
						//<< "no more boundary locations for the current level"
						//<< std::endl;

				return;
			}

			//LOG_ALL(imagelevelparserlog)
					//<< "found location " << newLocation
					//<< " on the boundary" << std::endl;

			// continue searching, if the boundary location was visited already
			if (_visited(newLocation.x(), newLocation.y())) {

				//LOG_ALL(imagelevelparserlog) << "this location was visited already" << std::endl;
				continue;
			}

			//LOG_ALL(imagelevelparserlog) << "going to the new location" << std::endl;

			// we found a not-yet-visited boundary location of the current 
			// level -- continue filling with it
			gotoLocation(newLocation, visitor);
			break;
		}
	}
}

template <typename Precision, typename ImageType>
template <typename VisitorType>
bool
ImageLevelParser<Precision, ImageType>::gotoHigherLevel(VisitorType& visitor) {

	point_type newLocation;
	Precision  newLevel;

	//LOG_ALL(imagelevelparserlog)
			//<< "trying to find smallest boundary location higher then "
			//<< (int)_currentLevel << std::endl;

	bool found = false;

	// find the lowest boundary location higher then the current level that has 
	// not been visited yet
	while (_boundaryLocations.popHigher(_currentLevel, newLocation, newLevel))
		if (!_visited(newLocation.x(), newLocation.y())) {

			found = true;
			break;
		}

	if (!found) {

		//LOG_ALL(imagelevelparserlog) << "nothing found, finishing up" << std::endl;

		// There are no more higher levels, we are done. End all the remaining 
		// open components (which are at least the component for level 
		// MaxValue).
		for (Precision level = _currentLevel;; level++) {

			endComponent(level, visitor);

			if (level == MaxValue)
				return false;
		}
	}

	//LOG_ALL(imagelevelparserlog)
			//<< "found boundary location " << newLocation
			//<< " with level " << (int)newLevel << std::endl;

	//LOG_ALL(imagelevelparserlog)
			//<< "ending all components in the range "
			//<< (int)_currentLevel << " - " << ((int)newLevel - 1)
			//<< std::endl;

	gotoLocation(newLocation, visitor);

	assert(_currentLevel == newLevel);

	return true;
}

template <typename Precision, typename ImageType>
template <typename VisitorType>
bool
ImageLevelParser<Precision, ImageType>::gotoLowerLevel(Precision referenceLevel, VisitorType& visitor) {

	point_type newLocation;
	Precision  newLevel;

	//LOG_ALL(imagelevelparserlog)
			//<< "trying to find lowest boundary location smaller then "
			//<< (int)referenceLevel << std::endl;

	// find the lowest boundary location higher then the reference level that 
	// has not been visited yet
	while (_boundaryLocations.popLowest(referenceLevel, newLocation, newLevel))
		if (!_visited(newLocation.x(), newLocation.y())) {

			//LOG_ALL(imagelevelparserlog)
					//<< "found boundary location " << newLocation
					//<< " with level " << (int)newLevel << std::endl;

			gotoLocation(newLocation, visitor);
			assert(_currentLevel == newLevel);
			return true;
		}

	return false;
}

template <typename Precision, typename ImageType>
template <typename VisitorType>
void
ImageLevelParser<Precision, ImageType>::beginComponent(Precision level, VisitorType& visitor) {

	_componentBegins.push(std::make_pair(level, _pixelList->end()));
	if (_parameters.spacedEdgeImage)
		_condensedComponentBegins.push(std::make_pair(level, _condensedPixelList->end()));

	visitor.newChildComponent(getOriginalValue(level));
}

template <typename Precision, typename ImageType>
template <typename VisitorType>
void
ImageLevelParser<Precision, ImageType>::endComponent(Precision level, VisitorType& visitor) {

	assert(_componentBegins.size() > 0);

	std::pair<Precision, PixelList::iterator> levelBegin;
	if (_parameters.spacedEdgeImage)
		levelBegin = _condensedComponentBegins.top();
	else
		levelBegin = _componentBegins.top();

	_componentBegins.pop();
	if (_parameters.spacedEdgeImage)
		_condensedComponentBegins.pop();

	PixelList::iterator begin = levelBegin.second;
	PixelList::iterator end;
	if (_parameters.spacedEdgeImage)
		end = _condensedPixelList->end();
	else
		end = _pixelList->end();

	assert(levelBegin.first == level);

	//LOG_ALL(imagelevelparserlog) << "ending component with level " << (int)level << std::endl;

	visitor.finalizeComponent(
			getOriginalValue(level),
			begin, end);
}

template <typename Precision, typename ImageType>
bool
ImageLevelParser<Precision, ImageType>::findNeighbor(
		Direction   direction,
		point_type& neighborLocation,
		Precision&  neighborLevel) {


	if (direction == Left) {

		//LOG_ALL(imagelevelparserlog) << "trying to go left" << std::endl;

		if (_currentLocation.x() == 0) {

			//LOG_ALL(imagelevelparserlog) << "\tout of bounds" << std::endl;
			return false;
		}

		neighborLocation = _currentLocation + point_type(-1,  0);
	}
	if (direction == Up) {

		//LOG_ALL(imagelevelparserlog) << "trying to go up" << std::endl;

		if (_currentLocation.y() == 0) {

			//LOG_ALL(imagelevelparserlog) << "\tout of bounds" << std::endl;
			return false;
		}

		neighborLocation = _currentLocation + point_type( 0, -1);
	}
	if (direction == Right) {

		//LOG_ALL(imagelevelparserlog) << "trying to go right" << std::endl;
		neighborLocation = _currentLocation + point_type( 1,  0);
	}

	if (direction == Down) {

		//LOG_ALL(imagelevelparserlog) << "trying to go down" << std::endl;
		neighborLocation = _currentLocation + point_type( 0,  1);
	}

	// out of bounds?
	if (neighborLocation.x() >= _image.width() ||
		neighborLocation.y() >= _image.height()) {

		//LOG_ALL(imagelevelparserlog) << "\tlocation " << neighborLocation << " is out of bounds" << std::endl;
		return false;
	}

	// already visited?
	if (_visited(neighborLocation.x(), neighborLocation.y())) {

		//LOG_ALL(imagelevelparserlog) << "\talready visited" << std::endl;
		return false;
	}

	//LOG_ALL(imagelevelparserlog)
			//<< "succeeded -- valid neighbor is "
			//<< neighborLocation << std::endl;

	// we're good
	neighborLevel = _image(neighborLocation.x(), neighborLocation.y());

	return true;
}

template <typename Precision,
          typename ImageType>
void
ImageLevelParser<Precision, ImageType>::discretizeImageImpl(const ImageType& image, std::false_type) {

	_image.reshape(image.shape());

	if (_parameters.minIntensity == 0 && _parameters.maxIntensity == 0) {

		image.minmax(&_min, &_max);

	} else {

		_min = _parameters.minIntensity;
		_max = _parameters.maxIntensity;
	}

	// in case the whole image has the same intensity
	if (_max - _min == 0) {

		_min = 0;
		_max = 1;
	}

	if (_max - _min > std::numeric_limits<Precision>::max())
		LOG_ERROR(imagelevelparserlog)
				<< "provided image has a range of " << (_max - _min)
				<< ", which does not fit into given precision" << std::endl;

	using namespace vigra::functor;

	if (_parameters.darkToBright)
		vigra::transformImage(
				srcImageRange(image),
				destImage(_image),
				// d = (v-min)/(max-min)*MAX
				( (Arg1()-Param(_min)) / Param(_max-_min) )*vigra::functor::Param(MaxValue));
	else // invert the image on-the-fly
		vigra::transformImage(
				srcImageRange(image),
				destImage(_image),
				// d = MAX - (v-min)/(max-min)*MAX
				Param(MaxValue) - ( (Arg1()-Param(_min)) / Param(_max-_min) )*Param(MaxValue));
}

template <typename Precision,
          typename ImageType>
void
ImageLevelParser<Precision, ImageType>::discretizeImageImpl(const ImageType& image, std::true_type) {

	_image.reshape(image.shape());

	LOG_DEBUG(imagelevelparserlog)
			<< "Requested parser precision and image are same type; "
			<< "ignoring min and max intensity parameters and using "
			<< "full precision range" << std::endl;

	_min = std::numeric_limits<Precision>::max();
	_max = std::numeric_limits<Precision>::max();

	using namespace vigra::functor;

	if (_parameters.darkToBright)
		vigra::copyImage(
				srcImageRange(image),
				destImage(_image));
	else // invert the image on-the-fly
		vigra::transformImage(
				srcImageRange(image),
				destImage(_image),
				(Param(_max) - Arg1()) + Param(_min));
}

template <typename Precision,
          typename ImageType>
typename ImageType::value_type
ImageLevelParser<Precision, ImageType>::getOriginalValueImpl(Precision value, std::false_type) {

	if (_parameters.darkToBright)
		// v = (d/MAX)*(max-min)+min
		return (static_cast<typename ImageType::value_type>(value)/MaxValue)*(_max - _min) + _min;
	else
		// v = ((MAX-d)/MAX)*(max-min)+min
		return (static_cast<typename ImageType::value_type>(MaxValue - value)/MaxValue)*(_max - _min) + _min;
}

template <typename Precision,
          typename ImageType>
typename ImageType::value_type
ImageLevelParser<Precision, ImageType>::getOriginalValueImpl(Precision value, std::true_type) {

	if (_parameters.darkToBright)
		return value;
	else
		return (std::numeric_limits<Precision>::max() - value) + std::numeric_limits<Precision>::min();
}



// SparseBoundaryLocations map implementation
template <typename Precision>
void
image_level_parser_detail::SparseBoundaryLocations<Precision>::push(
		const point_type& location,
		const Precision   level) {

	_boundaryLocations[level].push(location);
}

template <typename Precision>
bool
image_level_parser_detail::SparseBoundaryLocations<Precision>::pop(
		const Precision level,
		point_type&     boundaryLocation) {

	typename boundary_locations_type::iterator it = _boundaryLocations.find(level);
	if (it == _boundaryLocations.end())
		return false;

	return pop(*it, boundaryLocation);
}

template <typename Precision>
bool
image_level_parser_detail::SparseBoundaryLocations<Precision>::pop(
		typename boundary_locations_type::value_type& val,
		point_type& boundaryLocation) {

	typename boundary_locations_type::mapped_type& locations = val.second;
	if (locations.empty())
		return false;

	boundaryLocation = locations.top();
	locations.pop();

	return true;
}

template <typename Precision>
bool
image_level_parser_detail::SparseBoundaryLocations<Precision>::popLowest(
		const Precision level,
		point_type&     boundaryLocation,
		Precision&      boundaryLevel) {

	for (typename boundary_locations_type::iterator it = _boundaryLocations.begin(); (*it).first < level; ++it) {

		if (pop(*it, boundaryLocation)) {

			boundaryLevel = it->first;
			return true;
		}
	}

	return false;
}

template <typename Precision>
bool
image_level_parser_detail::SparseBoundaryLocations<Precision>::popHigher(
		const Precision level,
		point_type&     boundaryLocation,
		Precision&      boundaryLevel) {

	for (typename boundary_locations_type::iterator it = _boundaryLocations.upper_bound(level); it != _boundaryLocations.end(); ++it) {

		if (pop(*it, boundaryLocation)) {

			boundaryLevel = it->first;
			return true;
		}
	}

	return false;
}



// DenseBoundaryLocations vector implementation
template <typename Precision>
void
image_level_parser_detail::DenseBoundaryLocations<Precision>::push(
		const point_type& location,
		const Precision   level) {

	_boundaryLocations[level].push(location);
}

template <typename Precision>
bool
image_level_parser_detail::DenseBoundaryLocations<Precision>::pop(
		const Precision level,
		point_type&     boundaryLocation) {

	typename boundary_locations_type::value_type& locations = _boundaryLocations[level];

	if (locations.empty())
		return false;

	boundaryLocation = locations.top();
	locations.pop();

	return true;
}

template <typename Precision>
bool
image_level_parser_detail::DenseBoundaryLocations<Precision>::popLowest(
		const Precision level,
		point_type&     boundaryLocation,
		Precision&      boundaryLevel) {

	for (Precision l = 0; l < level; l++) {

		if (pop(l, boundaryLocation)) {

			boundaryLevel = l;
			return true;
		}
	}

	return false;
}

template <typename Precision>
bool
image_level_parser_detail::DenseBoundaryLocations<Precision>::popHigher(
		const Precision level,
		point_type&     boundaryLocation,
		Precision&      boundaryLevel) {

	if (level == MAX_LEVEL)
		return false;

	for (Precision l = level + 1;; l++) {

		if (pop(l, boundaryLocation)) {

			boundaryLevel = l;
			return true;
		}

		if (l == MAX_LEVEL)
			return false;
	}
}

#endif // IMAGEPROCESSING_IMAGE_LEVEL_PARSER_H__

