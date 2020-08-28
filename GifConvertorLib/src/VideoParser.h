#pragma once

#include "Parser.h"

#include <string>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}


class VideoParser : public Parser {
public:
	VideoParser(std::string file);

	std::unique_ptr<AVFrame, FrameDeleter> GetFrame() final;

	const AVCodecContext* const GetContext() const final;
	const int GetFramerate() const final;

	~VideoParser() final = default;
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

