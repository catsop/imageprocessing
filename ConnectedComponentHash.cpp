#include <algorithm>
#include <boost/functional/hash.hpp>
#include <util/foreach.h>
#include "ConnectedComponent.h"
#include "ConnectedComponentHash.h"

ConnectedComponentHash
hash_value(const ConnectedComponent& component) {

	ConnectedComponentHash hash = 0;

	// Calculate geometric hash value
	boost::hash_combine(hash, boost::hash_value(component.getBoundingBox().min().x()));
	boost::hash_combine(hash, boost::hash_value(component.getBoundingBox().min().y()));
	boost::hash_combine(hash, boost::hash_value(component.getBoundingBox().max().x()));
	boost::hash_combine(hash, boost::hash_value(component.getBoundingBox().max().y()));

	const ConnectedComponent::bitmap_type& thisBitmap = component.getBitmap();
	for (int x = 0; x < thisBitmap.width(); ++x)
	{
		for (int y = 0; y < thisBitmap.height(); ++y)
		{
			if (thisBitmap(x, y))
			{
				boost::hash_combine(hash, boost::hash_value(x + component.getBoundingBox().min().x()));
				boost::hash_combine(hash, boost::hash_value(y + component.getBoundingBox().min().y()));
			}
		}
	}

	return hash;
}
