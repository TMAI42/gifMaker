#include "GifMaker.h"

extern "C" {
#include <libavutil/mathematics.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <libavutil/pixdesc.h>
}


GifMaker::GifMaker(std::string filename, int width, int height, AVPixelFormat inputPixelFormat, int bitrate, int framerate)
	:outFormatContext(nullptr), codec(nullptr), frameCount(1) {

	av_register_all();

	/* allocate the output media context */
	avformat_alloc_output_context2(&outFormatContext, NULL, NULL, filename.c_str());

	if (!outFormatContext)
		avformat_alloc_output_context2(&outFormatContext, NULL, "gif", filename.c_str());

	if (!outFormatContext)
		throw std::exception("problems with context");

	outputFormat = outFormatContext->oformat;

	/* Add the audio and video streams using the default format codecs
	 * and initialize the codecs. */
	if (outputFormat->video_codec != AV_CODEC_ID_NONE) {
		/* find the video encoder */
		codec = avcodec_find_encoder(outputFormat->video_codec);

		if (!codec)
			throw std::exception("problems with codec");

		videoStream = avformat_new_stream(outFormatContext, codec);
		if (!videoStream)
			throw std::exception("problems with video stream");

		videoStream->id = outFormatContext->nb_streams - 1;
		outCodecContext = videoStream->codec;

		/* Some formats want stream headers to be separate. */
		if (outFormatContext->oformat->flags & AVFMT_GLOBALHEADER)
			outCodecContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
	}

	/* Now that all the parameters are set, we can open the audio and
	 * video codecs and allocate the necessary encode buffers.
	*/

	/* put sample parameters */
	outCodecContext->codec_id = outputFormat->video_codec;
	outCodecContext->bit_rate = bitrate;

	// resolution must be a multiple of two
	outCodecContext->width = width;
	outCodecContext->height = height;

	// frames per second
	outCodecContext->time_base = { 1,framerate };
	outCodecContext->pix_fmt = AV_PIX_FMT_RGB8;
	//c->gop_size = 10; // emit one intra frame every ten frames
	//c->max_b_frames=1;
	//

	int ret = 0;

	/* open it */
	AVDictionary* options = NULL;
	if ((ret = avcodec_open2(outCodecContext, codec, &options)) < 0)
		throw std::exception("could not open codec");

	/**
	* Init input frames
	* Get size of input frame for copy
	*/
	size = InitFrames(width, height, inputPixelFormat);


#ifdef _DEBUG
	av_dump_format(outFormatContext, 0, filename.c_str(), 1);
#endif 

	/* open the output file, if needed */

	if (!(outputFormat->flags & AVFMT_NOFILE))
		if ((ret = avio_open(&outFormatContext->pb, filename.c_str(), AVIO_FLAG_WRITE)) < 0)
			throw std::exception("Could not open file");

	/* Write the stream header, if any. */
	ret = avformat_write_header(outFormatContext, NULL);
	if (ret < 0)
		throw std::exception("Error occurred when opening output file");

	/**
	* Init pixel convertion context
	* Should be use after InitFrames()
	*/
	InitPixelConvertionContext();
}

GifMaker::GifMaker(std::string filename,const AVCodecContext* inputcodcecContext):
	GifMaker(filename, inputcodcecContext->width, inputcodcecContext->height, inputcodcecContext->pix_fmt,
	static_cast<int>(inputcodcecContext->bit_rate), inputcodcecContext->framerate.den){}

int GifMaker::InitFrames(int width, int height, AVPixelFormat inputPixelFormat) {
	/*we interested only in inputframe size*/
	int size{};

	/*Init input frame*/
	std::unique_ptr<AVFrame, FrameDeleter> pictureTmp1{ av_frame_alloc(), FrameDeleter() };
	pictureInput.swap(pictureTmp1);
	pictureInput->format = inputPixelFormat;

	if ((size = av_image_alloc(pictureInput->data, pictureInput->linesize,
		outCodecContext->width, outCodecContext->height, (AVPixelFormat)pictureInput->format, 32)) < 0)
			throw std::exception("can not allocate temp frame");


#ifdef _DEBUG
	else
		printf("allocated picture of size %d (ptr %x), linesize %d %d %d %d\n", size, pictureInput->data[0],
			pictureInput->linesize[0], pictureInput->linesize[1], pictureInput->linesize[2], pictureInput->linesize[3]);
#endif 

	pictureInput->height = height;
	pictureInput->width = width;

	/*Init aditional frame for color convertion */
	std::unique_ptr<AVFrame, FrameDeleter> pictureTmp2{ av_frame_alloc(), FrameDeleter() };
	pictureYUV420P.swap(pictureTmp2);
	pictureYUV420P->format = AV_PIX_FMT_RGB24;

	if ((av_image_alloc(pictureYUV420P->data, pictureYUV420P->linesize,
		outCodecContext->width, outCodecContext->height, (AVPixelFormat)pictureInput->format, 32)) < 0) 
			throw std::exception("can not allocate temp frame");
	

#ifdef _DEBUG
	else
		printf("allocated picture of size %d (ptr %x), linesize %d %d %d %d\n", 0, pictureYUV420P->data[0],
			pictureYUV420P->linesize[0], pictureYUV420P->linesize[1], pictureYUV420P->linesize[2], pictureYUV420P->linesize[3]);
#endif 

	pictureYUV420P->height = height;
	pictureYUV420P->width = width;

	return size;
}

void GifMaker::InitPixelConvertionContext() {

	/* get sws context for Input->YUV420P conversion */
	swsContextToYUV420P = sws_getContext(outCodecContext->width, outCodecContext->height, (AVPixelFormat)pictureInput->format,
		outCodecContext->width, outCodecContext->height, AV_PIX_FMT_YUV420P,
		SWS_BILINEAR, NULL, NULL, NULL);

	/* get sws context for YUV420P->RGB8 conversion */
	swsContextToRGB8 = sws_getContext(outCodecContext->width, outCodecContext->height, AV_PIX_FMT_YUV420P,
		outCodecContext->width, outCodecContext->height, AV_PIX_FMT_RGB8,
		SWS_BILINEAR, NULL, NULL, NULL);

	if (!swsContextToYUV420P || !swsContextToRGB8) 
		throw std::exception("Could not initialize the conversion context");
}

void GifMaker::AddFrame(std::shared_ptr<AVFrame> frame) {
	av_frame_copy(pictureInput.get(), frame.get());

	std::unique_ptr<AVFrame, FrameDeleter> picture{ av_frame_alloc(), FrameDeleter() };

	picture->data[0] = nullptr;
	picture->linesize[0] = -1;
	picture->format = outCodecContext->pix_fmt;

	if (av_image_alloc(picture->data, picture->linesize, outCodecContext->width, outCodecContext->height, (AVPixelFormat)picture->format, 32) < 0) 
		throw std::exception("error allocating with frames");
	

#ifdef _DEBUG
	else 
		printf("allocated picture of size %d (ptr %x), linesize %d %d %d %d\n", 0, picture->data[0],
			picture->linesize[0], picture->linesize[1], picture->linesize[2], picture->linesize[3]);
#endif

	picture->height = frame->height;
	picture->width = frame->width;

	/* convert input to YUV420P*/
	sws_scale(swsContextToYUV420P,
		pictureInput->data,
		pictureInput->linesize,
		0, outCodecContext->height,
		pictureYUV420P->data,
		pictureYUV420P->linesize);

	/* convert YUV420P to RGB8*/
	sws_scale(swsContextToRGB8,
		pictureYUV420P->data,
		pictureYUV420P->linesize,
		0, outCodecContext->height,
		picture->data,
		picture->linesize);

	/*init packet*/
	AVPacket pkt = { 0 };
	int got_packet;
	av_init_packet(&pkt);

	/* encode the image */
	int ret = -1;
	ret = avcodec_encode_video2(outCodecContext, &pkt, picture.get(), &got_packet);
	if (ret < 0) 
		throw std::exception("Error encoding video frame");
	
	/* If size is zero, it means the image was buffered. */
	if (!ret && got_packet && pkt.size) {
		pkt.stream_index = videoStream->index;
		/* Write the compressed frame to the media file. */
		ret = av_interleaved_write_frame(outFormatContext, &pkt);
	}

	picture->pts += av_rescale_q(1, videoStream->codec->time_base, videoStream->time_base);
	frameCount++;

}

void GifMaker::CloseWriteing() {
	av_write_trailer(outFormatContext);
	/* Close each codec. */

	avcodec_close(videoStream->codec);

	if (!(outputFormat->flags & AVFMT_NOFILE))
		/* Close the output file. */
		avio_close(outFormatContext->pb);

	/* free the stream */
	avformat_free_context(outFormatContext);

	frameCount = 0;
}


