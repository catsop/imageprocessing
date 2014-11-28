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
				boost::shared_ptr<Image>         image,
				unsigned int                     minSize,
				unsigned int                     maxSize) :
			_image(image),
			_minSize(minSize),
			_maxSize(maxSize),
			_prevBegin(0),
			_prevEnd(0) {}

		void setPixelList(boost::shared_ptr<PixelList> pixelList) { _pixelList = pixelList; }

		void finalizeComponent(
				float                        value,
				PixelList::const_iterator    begin,
				PixelList::const_iterator    end);

		boost::shared_ptr<ComponentTree::Node> getRoot() { assert(_roots.size() == 1); return _roots.top(); }

	private:

		// is the first range contained in the second?
		inline bool contained(
				const std::pair<PixelList::const_iterator, PixelList::const_iterator>& a,
				const std::pair<PixelList::const_iterator, PixelList::const_iterator>& b) {

			return (a.first >= b.first && a.second <= b.second);
		}

		boost::shared_ptr<Image>     _image;
		boost::shared_ptr<PixelList> _pixelList;

		// stack of open root nodes while constructing the tree
		std::stack<boost::shared_ptr<ComponentTree::Node> > _roots;

		unsigned int _minSize;
		unsigned int _maxSize;

		// extents of the previous component to detect changes
		PixelList::const_iterator _prevBegin;
		PixelList::const_iterator _prevEnd;
	};

	void updateOutputs();

	pipeline::Input<Image>                            _image;
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

	size_t size    = end - begin;
	bool   changed = (begin != _prevBegin || end != _prevEnd);

	_prevBegin = begin;
	_prevEnd   = end;

	bool valid = (size >= _minSize && (_maxSize == 0 || size < _maxSize) && changed);

	if (!valid)
		return;

	LOG_ALL(componenttreeextractorlog)
			<< "finalize component with value " << value << std::endl;

	// create a component tree node
	boost::shared_ptr<ComponentTree::Node> node
			= boost::make_shared<ComponentTree::Node>(
					boost::make_shared<ConnectedComponent>(
							_image,
							value,
							_pixelList,
							begin,
							end));

	// make all open root nodes that are subsets children of this component
	while (!_roots.empty() && contained(_roots.top()->getComponent()->getPixels(), std::make_pair(begin, end))) {

		node->addChild(_roots.top());
		_roots.top()->setParent(node);

		_roots.pop();
	}

	// put the new node on the stack
	_roots.push(node);
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

	unsigned int minSize = 0;
	unsigned int maxSize = 0;

	if (_parameters.isSet()) {

		minSize = _parameters->minSize;
		maxSize = _parameters->maxSize;
	}

	// create a new visitor
	ComponentVisitor visitor(_image.getSharedPointer(), minSize, maxSize);

	// create an image level parser
	typename ImageLevelParser<Precision>::Parameters parameters;
	parameters.darkToBright = (_parameters.isSet() ? _parameters->darkToBright : true);
	ImageLevelParser<Precision> parser(*_image, parameters);

	// let the visitor run over the components
	parser.parse(visitor);

	// set the root node in the component tree
	_componentTree->setRoot(visitor.getRoot());
}

#endif // IMAGEPROCESSING_COMPNENT_TREE_EXTRACTOR_H__

