

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
#include "RawWriter.h"

#include "ToGifConvertor.h"

//static AVFrame* alloc_picture(enum AVPixelFormat pix_fmt, int width, int height)
//{
//	AVFrame* picture;
//	int ret;
//	picture = av_frame_alloc();
//	if (!picture)
//		return NULL;
//	picture->format = pix_fmt;
//	picture->width = width;
//	picture->height = height;
//	/* allocate the buffers for the frame data */
//	ret = av_frame_get_buffer(picture, 0);
//	if (ret < 0) {
//		fprintf(stderr, "Could not allocate frame data.\n");
//		exit(1);
//	}
//	return picture;
//}
//
//void pgm_save(unsigned char* buf, int wrap, int xsize, int ysize, std::ofstream& file)
//{
//	// write header
//	file << "P5" << std::endl;
//	file << xsize << " " << ysize << std::endl;
//	file << 255 << std::endl;
//	// loop until all rows are written to file
//	for (int i = 0; i < ysize; i++)
//		file.write(reinterpret_cast<char*>(buf + i * wrap), xsize);
//		//fwrite(buf + i * wrap, 1, xsize, f);
//}
//
//void decode(std::shared_ptr<AVCodecContext> dec_ctx, std::shared_ptr<AVFrame> frame, std::shared_ptr<AVPacket> pkt, std::ofstream& file, GifMaker& encoder )
//{
//	int ret;
//
//	//send packet to decoder
//	ret = avcodec_send_packet(dec_ctx.get(), pkt.get());
//	if (ret < 0) {
//		std::cout << "Error sending a packet for decoding" << std::endl;
//		exit(1);
//	}
//	while (ret >= 0) {
//		// receive frame from decoder
//		// we may receive multiple frames or we may consume all data from decoder, then return to main loop
//		ret = avcodec_receive_frame(dec_ctx.get(), frame.get());
//		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
//			return;
//		else if (ret < 0) {
//			// something wrong, quit program
//			std::cout << "Error during decoding" << std::endl;
//			exit(1);
//		}
//		std::cout << "saving frame " << dec_ctx->frame_number << std::endl;
//		// send frame info to writing function
//		encoder.AddFrame(frame);
//
//		RawWriter::SaveFrame(frame, file);
//	}
//}

//int main()
//{
//
//	std::string infilename = "C:\\Users\\Andrey Strelchenko\\Downloads\\IMG_6240.MP4";
//	std::string outfilename = "C:\\Users\\Andrey Strelchenko\\Downloads\\pars\\frame.yuv";
//
//	int status;
//
//	AVFormatContext* fmt_ctx = avformat_alloc_context() ;
//
//	if (!fmt_ctx) {
//		throw std::exception("Couldn't created AVFormatContext");
//		return 0;
//	}
//
//	if (status = avformat_open_input(&fmt_ctx, infilename.c_str(), NULL, NULL) < 0)
//	{
//		av_log(NULL, AV_LOG_ERROR, "cannot open input file\n");
//		return 0;
//	}
//
//	std::unique_ptr<AVFormatContext, void(*)(AVFormatContext*)>
//		formatContext{ fmt_ctx, [](AVFormatContext* f) {avformat_close_input(&f); } };
//	fmt_ctx = nullptr;
//
//
//	if (status = avformat_find_stream_info(formatContext.get(), NULL) < 0)
//	{
//		av_log(NULL, AV_LOG_ERROR, "cannot get stream info\n");
//		return 0;
//	}
//
//
//	int VideoStreamIndex = -1;
//	for (int i = 0; i < formatContext->nb_streams; i++){
//
//		if (formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO){
//			VideoStreamIndex = i;
//			break;
//		}
//	}
//
//	if (VideoStreamIndex < 0){
//		av_log(NULL, AV_LOG_ERROR, "No video stream\n");
//		return 0;
//	}
//
//	//print detailed info for format
//	av_dump_format(formatContext.get(), VideoStreamIndex, infilename.c_str(), false);
//
//	AVCodec* Codec{ nullptr };
//	std::shared_ptr<AVCodecContext> codecContext{ avcodec_alloc_context3(Codec), avcodec_close };
//
//	if (status = avcodec_parameters_to_context(codecContext.get(), formatContext->streams[VideoStreamIndex]->codecpar) < 0){
//		av_log(NULL, AV_LOG_ERROR, "Cannot get codec parameters\n");
//		return 0;
//	}
//
//	Codec = avcodec_find_decoder(codecContext->codec_id);
//
//	if (!Codec){
//		av_log(NULL, AV_LOG_ERROR, "No decoder found\n");
//		return 0;
//	}
//
//	/**
//	*
//	* not needed if avcodec_alloc_context3() has non-Null argument
//	* but in this case codec initialise with codecContext data, so it should initialie after codec 
//	* avcodec_open2() should have the same codec as pased to avcodec_alloc_context3()
//	* if skip this this may couse problems with avcodec_send_packet()
//	*/
//	if (status = avcodec_open2(codecContext.get(), Codec, NULL) < 0){
//		av_log(NULL, AV_LOG_ERROR, "Cannot open video decoder\n");
//		return 0;
//	}
//
//	std::cout << std::endl << "Decoding codec is : " << Codec->name << std::endl;
//
//	std::shared_ptr<AVPacket> packet{ av_packet_alloc(), [](AVPacket* p) { av_packet_free(&p); } };
//	av_init_packet(packet.get());
//
//	if (!packet){
//		av_log(NULL, AV_LOG_ERROR, "Cannot init packet\n");
//		return 0;
//	}
//
//	std::shared_ptr<AVFrame> frame{ av_frame_alloc(), [](AVFrame* f) {av_frame_free(&f); } };
//
//	if (!frame){
//		av_log(NULL, AV_LOG_ERROR, "Cannot init frame\n");
//	}
//
//
//	std::ofstream outFile { outfilename , std::ios_base::binary | std::ios_base::out };
//
//	if (!outFile){
//		av_log(NULL, AV_LOG_ERROR, "Cannot open output file\n");
//		return 0;
//	}
//	
//
//	GifMaker a{ "C:\\Users\\Andrey Strelchenko\\Downloads\\pars\\text1.gif",
//		codecContext->width, codecContext->height, codecContext->pix_fmt,
//		static_cast<int>(codecContext->bit_rate), codecContext->framerate.den };
//
//
//	// main loop
//	while (true) {
//		// read an encoded packet from file
//		if (status = av_read_frame(formatContext.get(), packet.get()) < 0){
//			av_log(NULL, AV_LOG_ERROR, "cannot read frame");
//			break;
//		}
//
//		if (packet->stream_index == VideoStreamIndex){
//			decode(codecContext, frame, packet, outFile, a);
//		}
//
//		av_packet_unref(packet.get());
//	}
//
//	decode(codecContext, frame, NULL, outFile, a);
//
//	a.CloseWriteing();
//
//	system("PAUSE");
//
//	return 0;
//}


int main() {
	try {
		ToGifConvertor a("C:\\Users\\Andrey Strelchenko\\Downloads\\IMG_6240.MP4", "C:\\Users\\Andrey Strelchenko\\Downloads\\pars\\text12.gif");

		a.Convert();
	}
	catch (std::exception e) {
		std::cout << e.what();
	}


}