#pragma once

#include "ToGifConvertor.h"

namespace GifMakerCLI {

	public ref class Convertor : System::IDisposable {
	public:
		Convertor(System::String^ input, System::String^ output);

		void ToConvert();

	private:
		~Convertor();
		ToGifConvertor* current;
	};

}


