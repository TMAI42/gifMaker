#pragma once

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

#include <string>

#include "..\Maker.h"

class GifMaker: public Maker{
public:

    GifMaker(std::string filename, int width, int height, AVPixelFormat inputPixelFormat, int scale, int bitrate = 400000, int framerate = 25);
    GifMaker(std::string filename, const AVCodecContext* codcecContext, int farmerate, int scale = 1);


    void AddFrame(std::unique_ptr<AVFrame, FrameDeleter>) final ;

    void CloseWriteing() final ;

    ~GifMaker() final = default;

private:
    void InitOutFormatContext(std::string filename);
    void InitCodecAndVideoStream(int framerate);
    void InitSize(int width, int height, int sacle = 1);
    int InitFrames(int width, int height, AVPixelFormat inputPixelFormat);
    void InitPixelConvertionContext(int width, int height);

private:
    AVCodec* codec;
    int size, frameCount;
    int64_t pts;

    std::unique_ptr<AVFrame, FrameDeleter> pictureInput;
    std::unique_ptr<AVFrame, FrameDeleter> pictureYUV420P;

    SwsContext* swsContextToYUV420P;
    SwsContext* swsContextToRGB8;

    AVOutputFormat* outputFormat;
    std::unique_ptr<AVFormatContext, StreamCloser> outFormatContext;
    AVStream* videoStream;
    AVCodecContext* outCodecContext;

};


