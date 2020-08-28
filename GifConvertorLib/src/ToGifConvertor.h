#pragma once

#include <string>
#include <memory>

#include "Parser.h"
#include "Maker.h"
#include "Convertor.h"

class ToGifConvertor : public Convertor{

public:
	ToGifConvertor(std::string inputFilename, std::string outputFilename);

	void Convert() final ;

	~ToGifConvertor() final = default;
private:
	std::unique_ptr<Parser> parser;
	std::unique_ptr<Maker> builder;

};

