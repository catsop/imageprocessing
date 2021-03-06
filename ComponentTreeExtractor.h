#ifndef IMAGEPROCESSING_COMPNENT_TREE_EXTRACTOR_H__
#define IMAGEPROCESSING_COMPNENT_TREE_EXTRACTOR_H__

#include <pipeline/SimpleProcessNode.h>
#include <imageprocessing/ImageLevelParser.h>
#include "ComponentTree.h"
#include "ComponentTreeExtractorParameters.h"
#include "Image.h"
#include <util/Logger.h>

extern logger::LogChannel componenttreeextractorlog;

template <typename Precision = unsigned char, typename ImageType = IntensityImage>
class ComponentTreeExtractor : public pipeline::SimpleProcessNode<> {

public:

	ComponentTreeExtractor();

private:

	class ComponentVisitor : public ImageLevelParser<Precision, ImageType>::Visitor {

	public:

		ComponentVisitor(
				boost::shared_ptr<ImageType> image,
				unsigned int                 minSize,
				unsigned int                 maxSize,
				bool                         spacedEdgeImage) :
			_image(image),
			_minSize(minSize),
			_maxSize(maxSize),
			_spacedEdgeImage(spacedEdgeImage) {}

		void setPixelList(boost::shared_ptr<PixelList> pixelList) { _pixelList = pixelList; }

		inline void finalizeComponent(
				const typename ImageType::value_type value,
				PixelList::const_iterator            begin,
				PixelList::const_iterator            end);

		boost::shared_ptr<ComponentTree::Node> getRoot();

	private:

		// is the first range contained in the second?
		inline bool contained(
				const ConnectedComponent::PixelRange& a,
				const ConnectedComponent::PixelRange& b) {

			return (a.begin() >= b.begin() && a.end() <= b.end());
		}

		boost::shared_ptr<ImageType> _image;
		boost::shared_ptr<PixelList> _pixelList;

		// stack of open root nodes while constructing the tree
		std::stack<boost::shared_ptr<ComponentTree::Node> > _roots;

		unsigned int _minSize;
		unsigned int _maxSize;

		bool _spacedEdgeImage;

		// extents of the previous component to detect changes
		PixelList::const_iterator _prevBegin;
		PixelList::const_iterator _prevEnd;
	};

	void updateOutputs();

	pipeline::Input<ImageType>                        _image;
	pipeline::Input<ComponentTreeExtractorParameters<typename ImageType::value_type> > _parameters;
	pipeline::Output<ComponentTree>                   _componentTree;
};

////////////////////
// IMPLEMENTATION //
////////////////////

template <typename Precision, typename ImageType>
void
ComponentTreeExtractor<Precision, ImageType>::ComponentVisitor::finalizeComponent(
		const typename ImageType::value_type value,
		PixelList::const_iterator            begin,
		PixelList::const_iterator            end) {

	bool changed = (begin != _prevBegin || end != _prevEnd);

	_prevBegin = begin;
	_prevEnd   = end;

	if (!changed)
		return;

	PixelList::const_iterator::difference_type size = end - begin;

	bool wholeImage = (size == (_spacedEdgeImage ? _image->size() / 4 : _image->size()));
	bool validSize  = (size >= _minSize && (_maxSize == 0 || size < _maxSize));

	// we accept the whole image, even if it is not a valid size, to create a 
	// single root node
	if (!validSize && !wholeImage)
		return;

	LOG_ALL(componenttreeextractorlog)
			<< "finalize component with value " << value << std::endl;

	// For lossless storage of 64-bit values in components, e.g. labels,
	// just use the value's binary representation.
	// TODO: This is a hack to avoid templating ConnectedComponent on its value
	// type, since it would impact lots of code for something that has little
	// importance for its function.
	std::array<char, 8> ccValue;
	ccValue.fill(0);
	memcpy(ccValue.data(), &value, std::min(sizeof(value), ccValue.size()));

	// create a component tree node
	boost::shared_ptr<ComponentTree::Node> node
			= boost::make_shared<ComponentTree::Node>(
					boost::make_shared<ConnectedComponent>(
							ccValue,
							_pixelList,
							begin,
							end));

	// make all open root nodes that are subsets children of this component
	while (!_roots.empty() && contained(_roots.top()->getComponent()->getPixels(), ConnectedComponent::PixelRange(begin, end))) {

		node->addChild(_roots.top());
		_roots.top()->setParent(node);

		_roots.pop();
	}

	// put the new node on the stack
	_roots.push(node);
}

template <typename Precision, typename ImageType>
boost::shared_ptr<ComponentTree::Node>
ComponentTreeExtractor<Precision, ImageType>::ComponentVisitor::getRoot() {

	return _roots.top();
}

template <typename Precision, typename ImageType>
ComponentTreeExtractor<Precision, ImageType>::ComponentTreeExtractor() {

	registerInput(_image, "image");
	registerInput(_parameters, "parameters", pipeline::Optional);

	registerOutput(_componentTree, "component tree");
}

template <typename Precision, typename ImageType>
void
ComponentTreeExtractor<Precision, ImageType>::updateOutputs() {

	if (!_componentTree)
		_componentTree = new ComponentTree();
	else
		_componentTree->clear();

	LOG_DEBUG(componenttreeextractorlog) << "starting extraction" << std::endl;

	unsigned int minSize = 0;
	unsigned int maxSize = 0;
	bool spacedEdgeImage = false;

	if (_parameters.isSet()) {

		minSize = _parameters->minSize;
		maxSize = _parameters->maxSize;
		spacedEdgeImage = _parameters->spacedEdgeImage;
	}

	// create a new visitor
	ComponentVisitor visitor(_image.getSharedPointer(), minSize, maxSize, spacedEdgeImage);

	// create an image level parser
	typename ImageLevelParser<Precision, ImageType>::Parameters parameters;
	if (_parameters.isSet()) {

		parameters.darkToBright    = _parameters->darkToBright;
		parameters.minIntensity    = _parameters->minIntensity;
		parameters.maxIntensity    = _parameters->maxIntensity;
		parameters.spacedEdgeImage = _parameters->spacedEdgeImage;
	}

	if (_parameters->sameIntensityComponents) {

		ImageType separatedRegions = *_image;
		for (unsigned int y = 0; y < separatedRegions.height() - 1; y++)
			for (unsigned int x = 0; x < separatedRegions.width() - 1; x++) {

				typename ImageType::value_type value = separatedRegions(x, y),
				                               right = separatedRegions(x+1, y),
				                               down  = separatedRegions(x, y+1);

				if ((value != right && right != 0) || (value != down && down != 0))
					separatedRegions(x, y) = 0;
			}

		ImageLevelParser<Precision, ImageType> parser(separatedRegions, parameters);

		// let the visitor run over the components
		parser.parse(visitor);

	} else {

		ImageLevelParser<Precision, ImageType> parser(*_image, parameters);

		// let the visitor run over the components
		parser.parse(visitor);
	}

	// set the root node in the component tree
	_componentTree->setRoot(visitor.getRoot());

	LOG_DEBUG(componenttreeextractorlog)
			<< "extracted " << _componentTree->size()
			<< " components" << std::endl;
}

#endif // IMAGEPROCESSING_COMPNENT_TREE_EXTRACTOR_H__

