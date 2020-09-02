#pragma once

#include <memory>
#include <fstream>

class AVFrame;

class RawWriter {
public:
	static void SaveFrame(std::shared_ptr<AVFrame> farme, std::ofstream& file);
};

