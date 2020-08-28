#include "GifConvertor.h"

#include <string>
#include <msclr\marshal_cppstd.h>

GifMakerCLI::Convertor::~Convertor()
{
    delete current;
}

GifMakerCLI::Convertor::Convertor(System::String^ input, System::String^ output):
    current(new ToGifConvertor(msclr::interop::marshal_as<std::string>(input), msclr::interop::marshal_as<std::string>(output))){}

void GifMakerCLI::Convertor::ToConvert()
{
    current->Convert();
}
