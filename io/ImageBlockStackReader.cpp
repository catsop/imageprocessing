#include <pipeline/Value.h>
#include <imageprocessing/Image.h>
#include <imageprocessing/ImageStack.h>
#include <imageprocessing/io/ImageBlockFileReader.h>
#include "ImageBlockStackReader.h"

logger::LogChannel imageblockstackreaderlog("imageblockstackreaderlog", "[ImageBlockStackReader] ");

template <typename ImageType>
ImageBlockStackReader<ImageType>::ImageBlockStackReader()
{	
	_stackAssembler = boost::make_shared<StackAssembler>();
	registerInput(_block, "block");
	registerInput(_blockFactory, "factory");
	
	_block.registerCallback(&ImageBlockStackReader<ImageType>::onBlockModified, this);
	_blockFactory.registerCallback(&ImageBlockStackReader<ImageType>::onFactoryModified, this);
	
	// expose the result of the stack assembler
	registerOutput(_stackAssembler->getOutput(), "stack");
}

/*void
ImageBlockStackReader<ImageType>::updateOutputs()
{
	LOG_DEBUG(imageblockstackreaderlog) << "Updating outputs" << std::endl; 
	
}*/


template <typename ImageType>
ImageBlockStackReader<ImageType>::StackAssembler::StackAssembler() {

	registerInputs(_images, "images");
	registerOutput(_stack, "stack");
}

template <typename ImageType>
void
ImageBlockStackReader<ImageType>::StackAssembler::updateOutputs() {

	LOG_DEBUG(imageblockstackreaderlog) << "Stack Assembler updating outputs" << std::endl;
	_stack->clear();

	for (boost::shared_ptr<ImageType> image : _images)
		_stack->add(image);
}



template <typename ImageType>
void
ImageBlockStackReader<ImageType>::onBlockModified(const pipeline::InputSetBase&)
{
	LOG_DEBUG(imageblockstackreaderlog) << "Got block modified" << std::endl;
	setup();
}

template <typename ImageType>
void
ImageBlockStackReader<ImageType>::onFactoryModified(const pipeline::InputSetBase& )
{
	LOG_DEBUG(imageblockstackreaderlog) << "Got factory modified" << std::endl;
	setup();
}


template <typename ImageType>
void
ImageBlockStackReader<ImageType>::setup()
{
	if (_block.isSet() && _blockFactory.isSet())
	{
		int minZ = _block->min().z();
		int maxZ = _block->max().z();
		LOG_DEBUG(imageblockstackreaderlog) << "Clear readers" << std::endl;
		_blockReaders.clear();
		LOG_DEBUG(imageblockstackreaderlog) << "Clear stack assembler inputs" << std::endl; 
		_stackAssembler->clearInputs(0);
		for (int z = minZ; z < maxZ; ++z)
		{
			LOG_DEBUG(imageblockstackreaderlog) << "Adding input for z " << z << std::endl; 
			pipeline::Value<unsigned int> wrapZ(z);
			boost::shared_ptr<ImageBlockReader<ImageType> > reader = _blockFactory->getReader(z);
			_blockReaders.push_back(reader);
			reader->setInput("block", _block);
			
			reader->setInput("section", wrapZ);
			_stackAssembler->addInput(reader->getOutput("image"));
		}
	}
	else
	{
		LOG_DEBUG(imageblockstackreaderlog) << "Setup called, but inputs not ready." << std::endl;
	}
}

