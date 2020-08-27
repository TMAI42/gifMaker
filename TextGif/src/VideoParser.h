#pragma once

#include <string>
#include <memory>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

#include "Deleters.h"

class VideoParser{
public:
	VideoParser(std::string file);

	std::unique_ptr<AVFrame, FrameDeleter> GetFrame();

	const AVCodecContext* const GetContext() const;
	const int GetFramerate() const;

	~VideoParser() = default;
private:
	int FindVideoStream();
	void InitFormatContext();

private:
	std::string filename;

	AVCodec* Codec;
	std::unique_ptr<AVCodecContext, CodecContextDeleter> codecContext;

	std::unique_ptr<AVPacket, PacketDeleter> packet;

	std::unique_ptr<AVFormatContext, FormatContextDeleter> formatContext;

	int VideoStreamIndex;

};

