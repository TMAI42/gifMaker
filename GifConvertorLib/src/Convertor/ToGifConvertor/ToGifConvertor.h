#pragma once

#include <string>
#include <memory>

#include "..\..\parser\Parser.h"
#include "..\..\maker\Maker.h"
#include "..\Convertor.h"

class ToGifConvertor : public Convertor {

public:
	ToGifConvertor(std::string inputFilename, std::string outputFilename);

	ToGifConvertor(std::string inputFilename, std::string outputFilename, int scale, int slope);

	void Convert() final ;

	~ToGifConvertor() final = default;
private:
	std::unique_ptr<Parser> parser;
	std::unique_ptr<Maker> builder;

};

