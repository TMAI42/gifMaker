#include "ConvertorFactory.h"

#include "ToGifConvertor.h"

Convertor* ConvertorFactory::GetConvertor(std::string input, std::string output)
{
    return new ToGifConvertor(input, output);
}
