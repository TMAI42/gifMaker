#pragma once

#include "Convertor.h"
#include <string>

class ConvertorFactory{
public :
	static Convertor* GetConvertor(std::string input, std::string output);
};

