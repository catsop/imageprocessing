#ifndef IMAGEPROCESSING_COMPNENT_TREE_EXTRACTOR_H__
#define IMAGEPROCESSING_COMPNENT_TREE_EXTRACTOR_H__

#include <pipeline/SimpleProcessNode.h>
#include <imageprocessing/ImageLevelParser.h>
#include "ComponentTree.h"
#include "ComponentTreeExtractorParameters.h"
#include "Image.h"
#include <util/Logger.h>

extern logger::LogChannel componenttreeextractorlog;

template <typename Precision = unsigned char>
class ComponentTreeExtractor : public pipeline::SimpleProcessNode<> {

public:

	ComponentTreeExtractor();

private:

	class ComponentVisitor : public ImageLevelParser<Precision>::Visitor {

	public:

		ComponentVisitor(
				boost::shared_ptr<IntensityImage> image,
				unsigned int                      minSize,
				unsigned int                      maxSize,
				bool                              spacedEdgeImage) :
			_image(image),
			_minSize(minSize),
			_maxSize(maxSize),
			_spacedEdgeImage(spacedEdgeImage) {}

		void setPixelList(boost::shared_ptr<PixelList> pixelList) { _pixelList = pixelList; }

		inline void finalizeComponent(
				float                        value,
				PixelList::const_iterator    begin,
				PixelList::const_iterator    end);

		boost::shared_ptr<ComponentTree::Node> getRoot();

	private:

		// is the first range contained in the second?
		inline bool contained(
				const ConnectedComponent::PixelRange& a,
				const ConnectedComponent::PixelRange& b) {

			return (a.begin() >= b.begin() && a.end() <= b.end());
		}

		boost::shared_ptr<IntensityImage> _image;
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

	pipeline::Input<IntensityImage>                   _image;
	pipeline::Input<ComponentTreeExtractorParameters> _parameters;
	pipeline::Output<ComponentTree>                   _componentTree;
};

////////////////////
// IMPLEMENTATION //
////////////////////

template <typename Precision>
void
ComponentTreeExtractor<Precision>::ComponentVisitor::finalizeComponent(
		float                        value,
		PixelList::const_iterator    begin,
		PixelList::const_iterator    end) {

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

	// create a component tree node
	boost::shared_ptr<ComponentTree::Node> node
			= boost::make_shared<ComponentTree::Node>(
					boost::make_shared<ConnectedComponent>(
							value,
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

template <typename Precision>
boost::shared_ptr<ComponentTree::Node>
ComponentTreeExtractor<Precision>::ComponentVisitor::getRoot() {

	return _roots.top();
}

template <typename Precision>
ComponentTreeExtractor<Precision>::ComponentTreeExtractor() {

	registerInput(_image, "image");
	registerInput(_parameters, "parameters", pipeline::Optional);

	registerOutput(_componentTree, "component tree");
}

template <typename Precision>
void
ComponentTreeExtractor<Precision>::updateOutputs() {

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
	typename ImageLevelParser<Precision>::Parameters parameters;
	if (_parameters.isSet()) {

		parameters.darkToBright    = _parameters->darkToBright;
		parameters.minIntensity    = _parameters->minIntensity;
		parameters.maxIntensity    = _parameters->maxIntensity;
		parameters.spacedEdgeImage = _parameters->spacedEdgeImage;
	}

	if (_parameters->sameIntensityComponents) {

		IntensityImage separatedRegions = *_image;
		for (unsigned int y = 0; y < separatedRegions.height() - 1; y++)
			for (unsigned int x = 0; x < separatedRegions.width() - 1; x++) {

				float value = separatedRegions(x, y);
				float right = separatedRegions(x+1, y);
				float down  = separatedRegions(x, y+1);

				if ((value != right && right != 0) || (value != down && down != 0))
					separatedRegions(x, y) = 0;
			}

		ImageLevelParser<Precision> parser(separatedRegions, parameters);

		// let the visitor run over the components
		parser.parse(visitor);

	} else {

		ImageLevelParser<Precision> parser(*_image, parameters);

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
