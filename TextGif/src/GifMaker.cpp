#include "GifMaker.h"

extern "C" {
#include <libavutil/mathematics.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <libavutil/pixdesc.h>
}


GifMaker::GifMaker(std::string filename, int width, int height, int bitrate, int framerate)
	:outFormatContext(NULL), codec(NULL), frameCount(1) {

    av_register_all();

    /* allocate the output media context */
    avformat_alloc_output_context2(&outFormatContext, NULL, NULL, filename.c_str());

    if (!outFormatContext) {
        avformat_alloc_output_context2(&outFormatContext, NULL, "gif", filename.c_str());
    }
    if (!outFormatContext) {
        
        throw std::exception("problems with context");
    }
    outputFormat = outFormatContext->oformat;

    //AVPixelFormat supported_pix_fmt = AV_PIX_FMT_NONE;

    /* Add the audio and video streams using the default format codecs
     * and initialize the codecs. */
    if (outputFormat->video_codec != AV_CODEC_ID_NONE) {
        /* find the video encoder */
        codec = avcodec_find_encoder(outputFormat->video_codec);

        if (!codec) {
            throw std::exception("problems with codec");
        }
        else {
            //const AVPixelFormat* p = codec->pix_fmts;
            //while (p != NULL && *p != AV_PIX_FMT_NONE) {
            //    supported_pix_fmt = *p;
            //    ++p;
            //}
            //if (p == NULL || *p == AV_PIX_FMT_NONE) {
            //    if (outputFormat->video_codec == AV_CODEC_ID_RAWVIDEO) {
            //        supported_pix_fmt = AV_PIX_FMT_RGB24;
            //    }
            //    else {
            //        supported_pix_fmt = AV_PIX_FMT_YUV420P; /* default pix_fmt */
            //    }
            //}
        }

        videoStream = avformat_new_stream(outFormatContext, codec);
        if (!videoStream) {
            throw std::exception("problems with video stream");
        }
        videoStream->id = outFormatContext->nb_streams - 1;
        outCodecContext = videoStream->codec;

        /* Some formats want stream headers to be separate. */
        if (outFormatContext->oformat->flags & AVFMT_GLOBALHEADER)
            outCodecContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }

    /* Now that all the parameters are set, we can open the audio and
     * video codecs and allocate the necessary encode buffers. */
    {

        /* put sample parameters */
        outCodecContext->codec_id = outputFormat->video_codec;
        outCodecContext->bit_rate = bitrate;
        // resolution must be a multiple of two
        outCodecContext->width = width;
        outCodecContext->height = height;
        // frames per second
        outCodecContext->time_base = { 1,framerate };
        outCodecContext->pix_fmt = AV_PIX_FMT_RGB8;
        //        c->gop_size = 10; // emit one intra frame every ten frames
        //        c->max_b_frames=1;
        //
        //        { //try to get the default pix format
        //            AVCodecContext tmpcc;
        //            avcodec_get_context_defaults3(&tmpcc, c->codec);
        //            c->pix_fmt = (tmpcc.pix_fmt != AV_PIX_FMT_NONE) ? tmpcc.pix_fmt : AV_PIX_FMT_YUV420P;
        //        }
        int ret = 0;

        /* open it */
//        if(c->codec_id != AV_CODEC_ID_RAWVIDEO) {
        AVDictionary* options = NULL;
        if ((ret = avcodec_open2(outCodecContext, codec, &options)) < 0) {
            throw std::exception("could not open codec");
        }
        //        } else
        //            printf("raw video, no codec\n");

                /* alloc image and output buffer */
        picture.reset(av_frame_alloc());
        picture->data[0] = nullptr;
        picture->linesize[0] = -1;
        picture->format = outCodecContext->pix_fmt;

        ret = av_image_alloc(picture->data, picture->linesize, outCodecContext->width, outCodecContext->height, (AVPixelFormat)picture->format, 32);
        if (ret < 0) {
            throw std::exception("error allocating with frames");
        }
        else {
            printf("allocated picture of size %d (ptr %x), linesize %d %d %d %d\n", ret, picture->data[0], picture->linesize[0], picture->linesize[1], picture->linesize[2], picture->linesize[3]);
        }

        picture->height = height;
        picture->width = width;

        pictureYUV420P.reset(av_frame_alloc());
        pictureYUV420P->format = AV_PIX_FMT_RGB24;

        if ((ret = av_image_alloc(pictureYUV420P->data, pictureYUV420P->linesize, outCodecContext->width, outCodecContext->height, (AVPixelFormat)pictureYUV420P->format, 32)) < 0) {
            throw std::exception("can not allocate temp frame");
        }
        else
            printf("allocated picture of size %d (ptr %x), linesize %d %d %d %d\n", ret, pictureYUV420P->data[0], pictureYUV420P->linesize[0], pictureYUV420P->linesize[1], pictureYUV420P->linesize[2], pictureYUV420P->linesize[3]);

        pictureYUV420P->height = height;
        pictureYUV420P->width = width;

        size = ret;
    }

    
    av_dump_format(outFormatContext, 0, filename.c_str(), 1);
    /* open the output file, if needed */
    if (!(outputFormat->flags & AVFMT_NOFILE)) {
        int ret;
        if ((ret = avio_open(&outFormatContext->pb, filename.c_str(), AVIO_FLAG_WRITE)) < 0) {
            throw std::exception("Could not open file");
        }
    }
    /* Write the stream header, if any. */
    int ret = avformat_write_header(outFormatContext, NULL);
    if (ret < 0) {
        throw std::exception("Error occurred when opening output file");
    }

    /* get sws context for YUV420->RGB24 conversion */
    swsContext = sws_getContext(outCodecContext->width, outCodecContext->height, (AVPixelFormat)pictureYUV420P->format,
        outCodecContext->width, outCodecContext->height, (AVPixelFormat)picture->format,
                             SWS_BICUBIC, NULL, NULL, NULL);
    if (!swsContext) {
        throw std::exception("Could not initialize the conversion context");
    }

}

void GifMaker::AddFrame(std::shared_ptr<AVFrame> frame){
    /* copy the buffer */
    memcpy(pictureYUV420P->data[0], frame->data[0], size);
    

    /* convert RGB24 to YUV420 */
    auto t = sws_scale(swsContext,
        pictureYUV420P->data,
        pictureYUV420P->linesize,
        0, outCodecContext->height,
        picture->data,
        picture->linesize);

    int ret = -1;
    //if (outFormatContext->oformat->flags & AVFMT_RAWPICTURE) {
    //    /* Raw video case - directly store the picture in the packet */
        AVPacket pkt;
        av_init_packet(&pkt);
        pkt.flags |= AV_PKT_FLAG_KEY;
        pkt.stream_index = videoStream->index;
        pkt.data = picture->data[0];
        pkt.size = sizeof(picture->data);
        ret = av_interleaved_write_frame(outFormatContext, &pkt);
    //}
    //{
        //AVPacket pkt = { 0 };
        //int got_packet;
        //av_init_packet(&pkt);
        ///* encode the image */
       
        //ret = avcodec_encode_video2(outCodecContext, &pkt, picture.get(), &got_packet);
        //if (ret < 0) {
        //    throw std::exception("Error encoding video frame");
        //}
        ///* If size is zero, it means the image was buffered. */
        //if (!ret && got_packet && pkt.size) {
        //    pkt.stream_index = videoStream->index;
        //    /* Write the compressed frame to the media file. */
        //    ret = av_interleaved_write_frame(outFormatContext, &pkt);
        //}
        //else {
        //    ret = 0;
        //}
    //}
    picture->pts += av_rescale_q(1, videoStream->codec->time_base, videoStream->time_base);
    frameCount++;
}

void GifMaker::CloseWriteing(){
    av_write_trailer(outFormatContext);
    /* Close each codec. */

    avcodec_close(videoStream->codec);
    av_freep(&(picture->data[0]));

    if (!(outputFormat->flags & AVFMT_NOFILE))
        /* Close the output file. */
        avio_close(outFormatContext->pb);

    /* free the stream */
    avformat_free_context(outFormatContext);

    frameCount = 0;
}
