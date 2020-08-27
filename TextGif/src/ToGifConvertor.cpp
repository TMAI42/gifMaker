#include "ToGifConvertor.h"


ToGifConvertor::ToGifConvertor(std::string inputFilename, std::string outputFilename)
	:parcer(new VideoParcer(inputFilename)), builder(new GifMaker(outputFilename, parcer->GetContext())){}

void ToGifConvertor::Convert(){

	std::shared_ptr<AVFrame> frame = parcer->GetFrame();

	while (frame) {

		builder->AddFrame(frame);
		frame = parcer->GetFrame();
	} 

	builder->CloseWriteing();
}

