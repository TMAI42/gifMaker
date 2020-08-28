
#include "RawWriter.h"

extern "C" {
#include <libavcodec/avcodec.h>
}

void RawWriter::SaveFrame(std::shared_ptr<AVFrame> frame, std::ofstream& file) {

	// extract frame
	auto& buf = frame->data[0];
	auto& wrap = frame->linesize[0];
	auto& xSize = frame->width;
	auto& ySize = frame->height;
	
	// write header
	file << "P5" << std::endl;
	file << xSize << " " << ySize << std::endl;
	file << 255 << std::endl;
	// loop until all rows are written to file
	for (int i = 0; i < ySize; i++)
		file.write(reinterpret_cast<char*>(buf + i * wrap), xSize);
}
