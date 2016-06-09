#ifndef IMAGE_H__
#define IMAGE_H__

#include <cstdint>

#include <vigra/multi_array.hxx>

#include <pipeline/Data.h>
#include "DiscreteVolume.h"

#define EXPLICITLY_INSTANTIATE_COMMON_IMAGE_TYPES(T) \
		template class T<IntensityImage>;

/**
 * A vigra-compatible image class.
 */
template <typename T = float>
class Image : public pipeline::Data, public DiscreteVolume, public vigra::MultiArray<2, T> {

public:

	typedef vigra::MultiArray<2, T> array_type;

	Image(std::string identifier = std::string()) : _identifier(identifier) {};

	Image(size_t width, size_t height, T initialValue = 0.0f, std::string identifier = std::string()) :
		array_type(typename array_type::difference_type(width, height)),
		_identifier(identifier) {

		array_type::init(initialValue);
	}

	using array_type::operator=;

	/**
	 * The width of the image.
	 */
	typename array_type::difference_type_1 width() const { return array_type::size(0); }

	/**
	 * The height of the image.
	 */
	typename array_type::difference_type_1 height() const { return array_type::size(1); }

	/**
	 * Reshape the image.
	 */
	using array_type::reshape;

	/**
	 * Reshape the image.
	 */
	void reshape(size_t width, size_t height) {

		reshape(typename array_type::difference_type(width, height));
	}

	const std::string& getIdentifier() {

		return _identifier;
	}

	void setIdentifiyer(std::string identifier) {

		_identifier = identifier;
	}

protected:

	util::box<unsigned int,3> computeDiscreteBoundingBox() const override {

		return util::box<unsigned int,3>(0, 0, 0, array_type::size(0), array_type::size(1), 1);
	}

private:

	std::string _identifier;
};

typedef Image<bool> BinaryImage;
typedef Image<float> IntensityImage;
typedef Image<uint64_t> LabelImage;

#endif // IMAGE_H__
