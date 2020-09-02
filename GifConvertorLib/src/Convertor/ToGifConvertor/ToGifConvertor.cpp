#include "ToGifConvertor.h"

#include "..\..\parser\VideoParser\VideoParser.h"
#include "..\..\maker\GifMaker\GifMaker.h"


ToGifConvertor::ToGifConvertor(std::string inputFilename, std::string outputFilename)
	: parser(new VideoParser(inputFilename)), 
	builder(new GifMaker(outputFilename, parser->GetContext(), parser->GetFramerate())){}

ToGifConvertor::ToGifConvertor(std::string inputFilename, std::string outputFilename, int scale, int slope)
	: parser(new VideoParser(inputFilename)),
	builder(new GifMaker(outputFilename, parser->GetContext(), (parser->GetFramerate())/slope, scale)) {}

void ToGifConvertor::Convert() {

	auto fr = parser->GetFrame();
	std::unique_ptr<AVFrame, FrameDeleter> frame;
	frame.swap(fr);

	while (frame) {

		builder->AddFrame(std::move(frame));

		auto fr = parser->GetFrame();
		frame.swap(fr);
	} 

	builder->CloseWriteing();
}

