#pragma once

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

#include <memory>
#include <string>

class GifMaker{
public:

    GifMaker(std::string filename, int width, int height, AVPixelFormat inputPixelFormat, int bitrate = 400000, int framerate = 25 );

    void AddFrame(std::shared_ptr<AVFrame> frame);

    void CloseWriteing();

    ~GifMaker() = default;

private:
    AVCodec* codec;
    int size, frameCount;
    std::shared_ptr<AVFrame> pictureYUV420P;
    SwsContext* swsContext;
    AVOutputFormat* outputFormat;
    AVFormatContext* outFormatContext;
    AVStream* videoStream;
    AVCodecContext* outCodecContext;

};

