#pragma once

#include <string>
#include <memory>

#include "VideoParser.h"
#include "GifMaker.h"


class ToGifConvertor{

public:
	ToGifConvertor(std::string inputFilename, std::string outputFilename);

	void Convert();

	~ToGifConvertor() = default;
private:
	std::unique_ptr<VideoParser> parser;
	std::unique_ptr<GifMaker> builder;

};

