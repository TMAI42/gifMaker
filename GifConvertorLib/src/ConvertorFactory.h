#pragma once

#include "Convertor/Convertor.h"
#include <string>

class ConvertorFactory{
public :
	static Convertor* GetConvertor(std::string input, std::string output, int scale = 1 , int slope = 1);

};

