#pragma once
#include "ConvertorFactory.h"

namespace GifCLI {

	public ref class ConvertorExternal : System::IDisposable {
	public:
		ConvertorExternal(System::String^ input, System::String^ output, int scale, int slope);

		void ToConvert();

	private:
		~ConvertorExternal();
		Convertor* current;
	};
}
