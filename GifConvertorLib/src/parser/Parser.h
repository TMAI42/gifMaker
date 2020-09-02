#pragma once

#include "..\Deleters\Deleters.h"
#include <memory>

class Parser {
public :
	virtual std::unique_ptr<AVFrame, FrameDeleter> GetFrame() = 0;

	virtual const AVCodecContext* const GetContext() const = 0;
	virtual const int GetFramerate() const = 0;

	virtual ~Parser() = default;
};