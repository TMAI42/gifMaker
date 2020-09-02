#include "ConvertorFactory.h"

#include "Convertor/ToGifConvertor/ToGifConvertor.h"

Convertor* ConvertorFactory::GetConvertor(std::string input, std::string output, int scale, int slope) {
    return new ToGifConvertor(input, output, scale, slope);
}
