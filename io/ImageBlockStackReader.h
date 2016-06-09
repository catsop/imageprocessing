#ifndef IMAGE_BLOCK_STACK_READER_H__
#define IMAGE_BLOCK_STACK_READER_H__

#include <string>

#include <boost/filesystem.hpp>
#include <boost/shared_ptr.hpp>

#include <pipeline/all.h>
#include <imageprocessing/Image.h>
#include <imageprocessing/ImageStack.h>
#include <imageprocessing/io/ImageBlockFactory.h>
#include <util/box.hpp>

template <typename ImageType>
class ImageBlockStackReader : public pipeline::ProcessNode
{
public:
	ImageBlockStackReader();
	
private:
	//Like in ImageStackDirectoryReader
	class StackAssembler : public pipeline::SimpleProcessNode<>
	{

	public:

		StackAssembler();

	private:

		void updateOutputs();

		pipeline::Inputs<ImageType> _images;

		pipeline::Output<ImageStack<ImageType> > _stack;
	};

	//void updateOutputs();
	
	void onBlockModified(const pipeline::InputSetBase&);
	void onFactoryModified(const pipeline::InputSetBase&);

	void setup();
	
	std::vector<boost::shared_ptr<ImageBlockReader<ImageType> > > _blockReaders;
	
	boost::shared_ptr<StackAssembler> _stackAssembler;
	
	pipeline::Input<util::box<unsigned int, 3> > _block;
	pipeline::Input<ImageBlockFactory<ImageType> > _blockFactory;
};

#endif //IMAGE_BLOCK_STACK_READER_H__
