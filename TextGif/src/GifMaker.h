#pragma once

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

#include <memory>
#include <string>

#include "Deleters.h"

class GifMaker{
public:

    GifMaker(std::string filename, int width, int height, AVPixelFormat inputPixelFormat, int bitrate = 400000, int framerate = 25 );
    GifMaker(std::string filename, const AVCodecContext* codcecContext);

    void AddFrame(std::shared_ptr<AVFrame> frame);

    void CloseWriteing();

    ~GifMaker() = default;
private:
    int InitFrames(int width, int height, AVPixelFormat inputPixelFormat);
    void InitPixelConvertionContext();

private:
    AVCodec* codec;
    int size, frameCount;

    std::unique_ptr<AVFrame, FrameDeleter> pictureInput;
    std::unique_ptr<AVFrame, FrameDeleter> pictureYUV420P;

    SwsContext* swsContextToYUV420P;
    SwsContext* swsContextToRGB8;

    AVOutputFormat* outputFormat;
    AVFormatContext* outFormatContext;
    AVStream* videoStream;
    AVCodecContext* outCodecContext;

};


