

#include <stdio.h>

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/dict.h>

#include <libavcodec/avcodec.h>

#include <libavutil/opt.h>
#include <libavutil/imgutils.h>

#include <libswscale/swscale.h>
#include <libswresample/swresample.h>

}

#include <functional>
#include <memory>
#include <string>
#include <fstream>
#include <iostream>

#include "GifMaker.h"

static AVFrame* alloc_picture(enum AVPixelFormat pix_fmt, int width, int height)
{
	AVFrame* picture;
	int ret;
	picture = av_frame_alloc();
	if (!picture)
		return NULL;
	picture->format = pix_fmt;
	picture->width = width;
	picture->height = height;
	/* allocate the buffers for the frame data */
	ret = av_frame_get_buffer(picture, 0);
	if (ret < 0) {
		fprintf(stderr, "Could not allocate frame data.\n");
		exit(1);
	}
	return picture;
}

void GifMakerr(std::shared_ptr<AVFrame> frame, std::string filePath) {

	AVOutputFormat* outputFormat = nullptr;
	AVFormatContext* formatContext = nullptr;
	AVCodecContext* codecContext = nullptr;
	AVCodec* codec = nullptr;
	AVStream* stream = nullptr;
	SwsContext* gifContext = nullptr;

	std::string outfile = "C:\\Users\\Andrey Strelchenko\\Downloads\\pars\\text.gif";

	avformat_alloc_output_context2(&formatContext, NULL, NULL, filePath.c_str()); // i.e. filePath="C:/Users/.../qt_temp.Jv7868.mp4" 

	// Adding the video streams using the default format codecs and initializing the codecs...
	outputFormat = formatContext->oformat;
	if (outputFormat->video_codec != AV_CODEC_ID_NONE) {
		// Finding a registered encoder with a matching codec ID...
		codec = avcodec_find_encoder(outputFormat->video_codec);

		// Adding a new stream to a media file...
		stream = avformat_new_stream(formatContext, codec);
		stream->id = formatContext->nb_streams - 1;


		codecContext = avcodec_alloc_context3(codec);

		switch (codec->type) {
		case AVMEDIA_TYPE_VIDEO:
			codecContext->codec_id = outputFormat->video_codec; // here, outputFormat->video_codec should be AV_CODEC_ID_GIF
			codecContext->bit_rate = 400000;

			codecContext->width = 240;
			codecContext->height = 240;

			codecContext->pix_fmt = AV_PIX_FMT_RGB8;


			// Timebase: this is the fundamental unit of time (in seconds) in terms of which frame
			// timestamps are represented. For fixed-fps content, timebase should be 1/framerate
			// and timestamp increments should be identical to 1.
			stream->time_base = { 1, 15 }; // i.e. fps=1
			codecContext->time_base = stream->time_base;

			// Emit 1 intra frame every 12 frames at most
			codecContext->gop_size = 12;
			//codecContext->pix_fmt = AV_PIX_FMT_YUV420P;

			if (codecContext->codec_id == AV_CODEC_ID_H264) {
				av_opt_set(codecContext->priv_data, "preset", "slow", 0);
			}
			break;
		}

		if (formatContext->oformat->flags & AVFMT_GLOBALHEADER) {
			codecContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
		}
	}

	avcodec_open2(codecContext, codec, NULL);

	AVFrame* outFrame = alloc_picture(codecContext->pix_fmt, codecContext->width, codecContext->height );

	avcodec_parameters_from_context(stream->codecpar, codecContext);

	av_dump_format(formatContext, 0, filePath.data(), 1);

	if (!(outputFormat->flags & AVFMT_NOFILE)) {
		avio_open(&formatContext->pb, filePath.data(), AVIO_FLAG_WRITE);
	}

	// Writing the stream header, if any...
	avformat_write_header(formatContext, NULL);

	if (!gifContext) {
		gifContext = sws_getContext(frame->width, frame->height,
			AV_PIX_FMT_YUV420P,
			codecContext->width, codecContext->height,
			codecContext->pix_fmt,
			SWS_BICUBIC, NULL, NULL, NULL);

	}

	sws_scale(gifContext,
		(const uint8_t* const*)frame->data,
		frame->linesize,
		0,
		codecContext->height,
		outFrame->data,
		outFrame->linesize);


	AVPacket* packet = nullptr;
	int gotPacket = 0;

	av_init_packet(packet);
	
	// Packet data will be allocated by the encoder
	packet->data = nullptr;
	packet->size = 0;

	static int nextPts = 0;

	frame->pts = nextPts++; // nextPts starts at 0
	avcodec_encode_video2(codecContext, packet, outFrame, &gotPacket);

	if (gotPacket) {
		// Rescale output packet timestamp values from codec to stream timebase
		av_packet_rescale_ts(packet, codecContext->time_base, stream->time_base);
		packet->stream_index = stream->index;

		// Write the compressed frame to the media file.
		av_interleaved_write_frame(formatContext, packet);

		av_packet_unref(packet);
	}
	

	// Retrieving delayed frames if any...
// Note: mainly used for video generation, it might be useless for .gif.
	for (int gotOutput = 1; gotOutput;) {
		avcodec_encode_video2(codecContext, packet, NULL, &gotOutput);

		if (gotOutput) {
			// Rescale output packet timestamp values from codec to stream timebase
			av_packet_rescale_ts(packet, codecContext->time_base, stream->time_base);
			packet->stream_index = stream->index;

			// Write the compressed frame to the media file.
			av_interleaved_write_frame(formatContext, packet);
			av_packet_unref(packet);
		}
	}

	av_write_trailer(formatContext);

	avcodec_free_context(&codecContext);

	if (!(outputFormat->flags & AVFMT_NOFILE)) {
		// Closing the output file...
		avio_closep(&formatContext->pb);
	}

	avformat_free_context(formatContext);

}


void pgm_save(unsigned char* buf, int wrap, int xsize, int ysize, std::ofstream& file)
{
	// write header
	file << "P5" << std::endl;
	file << xsize << " " << ysize << std::endl;
	file << 255 << std::endl;
	// loop until all rows are written to file
	for (int i = 0; i < ysize; i++)
		file.write(reinterpret_cast<char*>(buf + i * wrap), xsize);
		//fwrite(buf + i * wrap, 1, xsize, f);
}

void decode(std::shared_ptr<AVCodecContext> dec_ctx, std::shared_ptr<AVFrame> frame, std::shared_ptr<AVPacket> pkt, std::ofstream& file, GifMaker& encoder )
{
	char buf[1024];
	int ret;

	//send packet to decoder
	ret = avcodec_send_packet(dec_ctx.get(), pkt.get());
	if (ret < 0) {
		std::cout << "Error sending a packet for decoding" << std::endl;
		exit(1);
	}
	while (ret >= 0) {
		// receive frame from decoder
		// we may receive multiple frames or we may consume all data from decoder, then return to main loop
		ret = avcodec_receive_frame(dec_ctx.get(), frame.get());
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
			return;
		else if (ret < 0) {
			// something wrong, quit program
			std::cout << "Error during decoding" << std::endl;
			exit(1);
		}
		std::cout << "saving frame " << dec_ctx->frame_number << std::endl;
		// send frame info to writing function
		//GifMaker(frame, "C:\\Users\\Andrey Strelchenko\\Downloads\\pars\\text.gif");
		encoder.AddFrame(frame);
		pgm_save(frame->data[0], frame->linesize[0], frame->width, frame->height, file);
	}
}

int main()
{
	AVFormatContext* fmt_ctx { nullptr };
	AVCodec* Codec { nullptr };

	std::shared_ptr<AVCodecContext> codecContext{ nullptr, avcodec_close };

	

	std::shared_ptr<AVPacket> packet{ nullptr, [](AVPacket* p) { av_packet_free(&p); } };

	std::string infilename = "C:\\Users\\Andrey Strelchenko\\Downloads\\video.mp4";
	std::string outfilename = "C:\\Users\\Andrey Strelchenko\\Downloads\\pars\\frame.yuv";

	int status;


	fmt_ctx = avformat_alloc_context();

	if (!fmt_ctx) {
		throw std::exception("Couldn't created AVFormatContext");
		return 0;
	}

	if (status = avformat_open_input(&fmt_ctx, infilename.c_str(), NULL, NULL) < 0)
	{
		av_log(NULL, AV_LOG_ERROR, "cannot open input file\n");
		return 0;
	}


	std::unique_ptr<AVFormatContext, void(*)(AVFormatContext*)>
		formatContext{ fmt_ctx, [](AVFormatContext* f) {avformat_close_input(&f); } };
	fmt_ctx = nullptr;


	if (status = avformat_find_stream_info(formatContext.get(), NULL) < 0)
	{
		av_log(NULL, AV_LOG_ERROR, "cannot get stream info\n");
		return 0;
	}


	int VideoStreamIndex = -1;
	for (int i = 0; i < formatContext->nb_streams; i++){

		if (formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO){
			VideoStreamIndex = i;
			break;
		}
	}

	if (VideoStreamIndex < 0){
		av_log(NULL, AV_LOG_ERROR, "No video stream\n");
		return 0;
	}

	//print detailed info for format
	av_dump_format(formatContext.get(), VideoStreamIndex, infilename.c_str(), false);

	codecContext.reset(avcodec_alloc_context3(Codec));

	if (status = avcodec_parameters_to_context(codecContext.get(), formatContext->streams[VideoStreamIndex]->codecpar) < 0){
		av_log(NULL, AV_LOG_ERROR, "Cannot get codec parameters\n");
		return 0;
	}

	Codec = avcodec_find_decoder(codecContext->codec_id);

	if (!Codec){
		av_log(NULL, AV_LOG_ERROR, "No decoder found\n");
		return 0;
	}

	/**
	*
	* not needed if avcodec_alloc_context3() has non-Null argument
	* but in this case codec initialise with codecContext data, so it should initialie after codec 
	* avcodec_open2() should have the same codec as pased to avcodec_alloc_context3()
	* if skip this thismay couse problems with avcodec_send_packet()
	*/
	if (status = avcodec_open2(codecContext.get(), Codec, NULL) < 0){
		av_log(NULL, AV_LOG_ERROR, "Cannot open video decoder\n");
		return 0;
	}

	std::cout << std::endl << "Decoding codec is : " << Codec->name << std::endl;

	packet.reset(av_packet_alloc());

	av_init_packet(packet.get());

	if (!packet){
		av_log(NULL, AV_LOG_ERROR, "Cannot init packet\n");
		return 0;
	}

	std::shared_ptr<AVFrame> frame{ av_frame_alloc(), [](AVFrame* f) {av_frame_free(&f); } };

	if (!frame){
		av_log(NULL, AV_LOG_ERROR, "Cannot init frame\n");
	}


	//std::ifstream inFile { infilename , std::ios_base::binary | std::ios_base::in };

	//if (!inFile){
	//	av_log(NULL, AV_LOG_ERROR, "Cannot open input file\n");
	//	return 0;
	//}

	std::ofstream outFile { outfilename , std::ios_base::binary | std::ios_base::out };

	if (!outFile){
		av_log(NULL, AV_LOG_ERROR, "Cannot open output file\n");
		return 0;
	}

	

	GifMaker a{ "C:\\Users\\Andrey Strelchenko\\Downloads\\pars\\text.gif", codecContext->width, codecContext->height, codecContext->pix_fmt };


	// main loop
	while (true) {
		// read an encoded packet from file
		if (status = av_read_frame(formatContext.get(), packet.get()) < 0){
			av_log(NULL, AV_LOG_ERROR, "cannot read frame");
			break;
		}

		if (packet->stream_index == VideoStreamIndex){
			decode(codecContext, frame, packet, outFile, a);
		}

		av_packet_unref(packet.get());
	}

	decode(codecContext, frame, NULL, outFile, a);

	a.CloseWriteing();

	system("PAUSE");

	return 0;
}



//void aaaa(std::string filePath) {
//	AVOutputFormat* outputFormat = nullptr;
//	AVFormatContext* formatContext = nullptr;
//	AVCodec* codec = nullptr;
//	AVStream* stream = nullptr;
//
//	avformat_alloc_output_context2(&formatContext, NULL, NULL, filePath.c_str()); // i.e. filePath="C:/Users/.../qt_temp.Jv7868.mp4" 
//
//	// Adding the video streams using the default format codecs and initializing the codecs...
//	outputFormat = formatContext->oformat;
//	if (outputFormat->video_codec != AV_CODEC_ID_NONE) {
//		// Finding a registered encoder with a matching codec ID...
//		codec = avcodec_find_encoder(outputFormat->video_codec);
//
//		// Adding a new stream to a media file...
//		stream = avformat_new_stream(formatContext, codec);
//		stream->id = formatContext->nb_streams - 1;
//
//
//		AVCodecContext* codecContext = avcodec_alloc_context3(codec);
//
//		switch (codec->type) {
//		case AVMEDIA_TYPE_VIDEO:
//			codecContext->codec_id = outputFormat->video_codec; // here, outputFormat->video_codec should be AV_CODEC_ID_GIF
//			codecContext->bit_rate = 400000;
//
//			codecContext->width = 1240;
//			codecContext->height = 874;
//
//			codecContext->pix_fmt = AV_PIX_FMT_RGB8;
//
//
//				// Timebase: this is the fundamental unit of time (in seconds) in terms of which frame
//				// timestamps are represented. For fixed-fps content, timebase should be 1/framerate
//				// and timestamp increments should be identical to 1.
//				stream->time_base = { 1, 15 }; // i.e. fps=1
//			codecContext->time_base = stream->time_base;
//
//			// Emit 1 intra frame every 12 frames at most
//			codecContext->gop_size = 12;
//			codecContext->pix_fmt = AV_PIX_FMT_YUV420P;
//
//			if (codecContext->codec_id == AV_CODEC_ID_H264) {
//				av_opt_set(codecContext->priv_data, "preset", "slow", 0);
//			}
//			break;
//		}
//
//		if (formatContext->oformat->flags & AVFMT_GLOBALHEADER) {
//			codecContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
//		}
//	}
//
//	avcodec_open2(codecContext, codec, NULL);
//
//	// Here we need 3 frames. Basically, the QImage is firstly extracted in AV_PIX_FMT_BGRA.
//	// We need then to convert it to AV_PIX_FMT_RGB8 which is required by the .gif format.
//	// If we do that directly, there will be some artefacts and bad effects... to prevent that
//	// we convert FIRST AV_PIX_FMT_BGRA into AV_PIX_FMT_YUV420P THEN into AV_PIX_FMT_RGB8.
//	frame = allocPicture(codecContext->width, codecContext->height, codecContext->pix_fmt); // here, codecContext->pix_fmt should be AV_PIX_FMT_RGB8
//	tmpFrame = allocPicture(codecContext->width, codecContext->height, AV_PIX_FMT_BGRA);
//	yuvFrame = allocPicture(codecContext->width, codecContext->height, AV_PIX_FMT_YUV420P);
//
//	avcodec_parameters_from_context(stream->codecpar, codecContext);
//
//	av_dump_format(formatContext, 0, filePath.c_str(), 1);
//
//	if (!(outputFormat->flags & AVFMT_NOFILE)) {
//		avio_open(&formatContext->pb, filePath.c_str(), AVIO_FLAG_WRITE);
//	}
//
//	// Writing the stream header, if any...
//	avformat_write_header(formatContext, NULL);
//
//
//	// -> parameter: QImage image
//	const int width = image.width();
//	const int height = image.height();
//
//	// When we pass a frame to the encoder, it may keep a reference to it internally;
//	// make sure we do not overwrite it here!
//	av_frame_make_writable(tmpFrame);
//
//	// Converting QImage to AV_PIX_FMT_BGRA AVFrame ...
//	for (int y = 0; y < height; y++) {
//		const uint8_t* scanline = image.scanLine(y);
//
//		for (int x = 0; x < width * 4; x++) {
//			tmpFrame->data[0][y * tmpFrame->linesize[0] + x] = scanline[x];
//		}
//	}
//
//	// Make sure to clear the frame. It prevents a bug that displays only the
//	// first captured frame on the GIF export.
//	if (frame) {
//		av_frame_free(&frame);
//		frame = nullptr;
//	}
//	frame = allocPicture(codecContext->width, codecContext->height, codecContext->pix_fmt);
//
//	if (yuvFrame) {
//		av_frame_free(&yuvFrame);
//		yuvFrame = nullptr;
//	}
//	yuvFrame = allocPicture(codecContext->width, codecContext->height, AV_PIX_FMT_YUV420P);
//
//	// Converting BGRA -> YUV420P...
//	if (!swsCtx) {
//		swsCtx = sws_getContext(width, height,
//			AV_PIX_FMT_BGRA,
//			width, height,
//			AV_PIX_FMT_YUV420P,
//			swsFlags, NULL, NULL, NULL);
//	}
//
//	// ...then converting YUV420P -> RGB8 (natif GIF format pixel)
//	if (!swsGIFCtx) {
//		swsGIFCtx = sws_getContext(width, height,
//			AV_PIX_FMT_YUV420P,
//			codecContext->width, codecContext->height,
//			codecContext->pix_fmt,
//			this->swsFlags, NULL, NULL, NULL);
//	}
//
//	// This double scaling prevent some artifacts on the GIF and improve
//	// significantly the display quality
//	sws_scale(swsCtx,
//		(const uint8_t* const*)tmpFrame->data,
//		tmpFrame->linesize,
//		0,
//		codecContext->height,
//		yuvFrame->data,
//		yuvFrame->linesize);
//	sws_scale(swsGIFCtx,
//		(const uint8_t* const*)yuvFrame->data,
//		yuvFrame->linesize,
//		0,
//		codecContext->height,
//		frame->data,
//		frame->linesize);
//
//	...
//
//		AVPacket packet;
//	int gotPacket = 0;
//
//	av_init_packet(&packet);
//
//	// Packet data will be allocated by the encoder
//	packet.data = NULL;
//	packet.size = 0;
//
//	frame->pts = nextPts++; // nextPts starts at 0
//	avcodec_encode_video2(codecContext, &packet, frame, &gotPacket);
//
//	if (gotPacket) {
//		// Rescale output packet timestamp values from codec to stream timebase
//		av_packet_rescale_ts(packet, *codecContext->time_base, stream->time_base);
//		packet->stream_index = stream->index;
//
//		// Write the compressed frame to the media file.
//		av_interleaved_write_frame(formatContext, packet);
//
//		av_packet_unref(&this->packet);
//	}
//}


//
//#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>
//
//#include <libavcodec/avcodec.h>
//
//#include <libavutil/opt.h>
//#include <libavutil/imgutils.h>
//
//static void encode(AVCodecContext* enc_ctx, AVFrame* frame, AVPacket* pkt,
//    FILE* outfile)
//{
//    int ret;
//
//    /* send the frame to the encoder */
//    if (frame)
//        printf("Send frame %3"PRId64"\n", frame->pts);
//
//    ret = avcodec_send_frame(enc_ctx, frame);
//    if (ret < 0) {
//        fprintf(stderr, "Error sending a frame for encoding\n");
//        exit(1);
//    }
//
//    while (ret >= 0) {
//        ret = avcodec_receive_packet(enc_ctx, pkt);
//        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
//            return;
//        else if (ret < 0) {
//            fprintf(stderr, "Error during encoding\n");
//            exit(1);
//        }
//
//        printf("Write packet %3"PRId64" (size=%5d)\n", pkt->pts, pkt->size);
//        fwrite(pkt->data, 1, pkt->size, outfile);
//        av_packet_unref(pkt);
//    }
//}
//
//int main(int argc, char** argv)
//{
//    const char* filename, * codec_name;
//    const AVCodec* codec;
//    AVCodecContext* c = NULL;
//    int i, ret, x, y;
//    FILE* f;
//    AVFrame* frame;
//    AVPacket* pkt;
//    uint8_t endcode[] = { 0, 0, 1, 0xb7 };
//
//    if (argc <= 2) {
//        fprintf(stderr, "Usage: %s <output file> <codec name>\n", argv[0]);
//        exit(0);
//    }
//    filename = argv[1];
//    codec_name = argv[2];
//
//    /* find the mpeg1video encoder */
//    codec = avcodec_find_encoder_by_name(codec_name);
//    if (!codec) {
//        fprintf(stderr, "Codec '%s' not found\n", codec_name);
//        exit(1);
//    }
//
//    c = avcodec_alloc_context3(codec);
//    if (!c) {
//        fprintf(stderr, "Could not allocate video codec context\n");
//        exit(1);
//    }
//
//    pkt = av_packet_alloc();
//    if (!pkt)
//        exit(1);
//
//    /* put sample parameters */
//    c->bit_rate = 400000;
//    /* resolution must be a multiple of two */
//    c->width = 352;
//    c->height = 288;
//    /* frames per second */
//    c->time_base = { 1, 25 };
//    c->framerate = { 25, 1 };
//
//    /* emit one intra frame every ten frames
//     * check frame pict_type before passing frame
//     * to encoder, if frame->pict_type is AV_PICTURE_TYPE_I
//     * then gop_size is ignored and the output of encoder
//     * will always be I frame irrespective to gop_size
//     */
//    c->gop_size = 10;
//    c->max_b_frames = 1;
//    c->pix_fmt = AV_PIX_FMT_YUV420P;
//
//    if (codec->id == AV_CODEC_ID_H264)
//        av_opt_set(c->priv_data, "preset", "slow", 0);
//
//    /* open it */
//    ret = avcodec_open2(c, codec, NULL);
//    if (ret < 0) {
//        fprintf(stderr, "Could not open codec: %s\n", av_err2str(ret));
//        exit(1);
//    }
//
//    f = fopen(filename, "wb");
//    if (!f) {
//        fprintf(stderr, "Could not open %s\n", filename);
//        exit(1);
//    }
//
//    frame = av_frame_alloc();
//    if (!frame) {
//        fprintf(stderr, "Could not allocate video frame\n");
//        exit(1);
//    }
//    frame->format = c->pix_fmt;
//    frame->width = c->width;
//    frame->height = c->height;
//
//    ret = av_frame_get_buffer(frame, 0);
//    if (ret < 0) {
//        fprintf(stderr, "Could not allocate the video frame data\n");
//        exit(1);
//    }
//
//    /* encode 1 second of video */
//    for (i = 0; i < 25; i++) {
//        fflush(stdout);
//
//        /* make sure the frame data is writable */
//        ret = av_frame_make_writable(frame);
//        if (ret < 0)
//            exit(1);
//
//        /* prepare a dummy image */
//        /* Y */
//        for (y = 0; y < c->height; y++) {
//            for (x = 0; x < c->width; x++) {
//                frame->data[0][y * frame->linesize[0] + x] = x + y + i * 3;
//            }
//        }
//
//        /* Cb and Cr */
//        for (y = 0; y < c->height / 2; y++) {
//            for (x = 0; x < c->width / 2; x++) {
//                frame->data[1][y * frame->linesize[1] + x] = 128 + y + i * 2;
//                frame->data[2][y * frame->linesize[2] + x] = 64 + x + i * 5;
//            }
//        }
//
//        frame->pts = i;
//
//        /* encode the image */
//        encode(c, frame, pkt, f);
//    }
//
//    /* flush the encoder */
//    encode(c, NULL, pkt, f);
//
//    /* add sequence end code to have a real MPEG file */
//    if (codec->id == AV_CODEC_ID_MPEG1VIDEO || codec->id == AV_CODEC_ID_MPEG2VIDEO)
//        fwrite(endcode, 1, sizeof(endcode), f);
//    fclose(f);
//
//    avcodec_free_context(&c);
//    av_frame_free(&frame);
//    av_packet_free(&pkt);
//
//    return 0;
//}
