#include <algorithm>
#include <boost/functional/hash.hpp>
#include <util/foreach.h>
#include "ConnectedComponent.h"
#include "ConnectedComponentHash.h"

ConnectedComponentHash
hash_value(const ConnectedComponent& component) {

	ConnectedComponentHash hash = 0;

	// Calculate geometric hash value
	boost::hash_combine(hash, boost::hash_value(component.getBoundingBox().minX));
	boost::hash_combine(hash, boost::hash_value(component.getBoundingBox().minY));
	boost::hash_combine(hash, boost::hash_value(component.getBoundingBox().maxX));
	boost::hash_combine(hash, boost::hash_value(component.getBoundingBox().maxY));

	const ConnectedComponent::bitmap_type& thisBitmap = component.getBitmap();
	for (int x = 0; x < thisBitmap.width(); ++x)
	{
		for (int y = 0; y < thisBitmap.height(); ++y)
		{
			if (thisBitmap(x, y))
			{
				boost::hash_combine(hash, boost::hash_value(x + component.getBoundingBox().minX));
				boost::hash_combine(hash, boost::hash_value(y + component.getBoundingBox().minY));
			}
		}
	}

	return hash;
}
