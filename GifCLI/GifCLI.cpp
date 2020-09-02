#include "pch.h"

#include "GifCLI.h"

using namespace GifCLI;

#include <string>
#include <msclr\marshal_cppstd.h>

ConvertorExternal::~ConvertorExternal() {
    delete current;
}

ConvertorExternal::ConvertorExternal(System::String^ input, System::String^ output, int scale, int slope) :
    current(ConvertorFactory::GetConvertor(msclr::interop::marshal_as<std::string>(input), msclr::interop::marshal_as<std::string>(output), scale, slope)) {}

void ConvertorExternal::ToConvert() {
    current->Convert();
}

