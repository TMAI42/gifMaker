#pragma once

#include <string>
#include <memory>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

#include "Deleters.h"

class VideoParcer{
public:
	VideoParcer(std::string file);

	std::shared_ptr<AVFrame> GetFrame();
	const AVCodecContext* const GetContext() const;
private:
	int FindVideoStream();

private:
	std::string filename;

	AVCodec* Codec;
	std::unique_ptr<AVCodecContext, CodecContextDeleter> codecContext;

	std::unique_ptr<AVPacket, PacketDeleter> packet;
	std::shared_ptr<AVFrame> frame;

	std::unique_ptr<AVFormatContext, FormatContextDeleter> formatContext;


	int VideoStreamIndex;

};

