#pragma once

#ifndef VFORMAT_AVFORMAT_H
extern "C" {
#include <libavformat/avformat.h>
}
#endif

struct FrameDeleter{

    void operator()(AVFrame* frame) {
        if (frame)
            av_frame_free(&frame);
    }
};

struct FormatContextDeleter {

    void operator()(AVFormatContext* formatContext) {
        if (formatContext) {
            avformat_close_input(&formatContext);
        }
    }
};

struct StreamCloser {

    void operator()(AVFormatContext* formatContext) {
        if (formatContext) {
            avformat_free_context(formatContext);
        }
    }
};

struct CodecContextDeleter {

    void operator()(AVCodecContext* codecContext) {
        if (codecContext)
            avcodec_close(codecContext);
    }
};

struct PacketDeleter {

    void operator()(AVPacket* packet) {
        if (packet)
            av_packet_free(&packet);
    }
};
