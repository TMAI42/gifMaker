#include "VideoParcer.h"

#ifdef _DEBUG
#include <iostream>
#endif 


VideoParcer::VideoParcer(std::string file)
	:filename(file), Codec(nullptr), codecContext(avcodec_alloc_context3(Codec), CodecContextDeleter()), 
	packet(av_packet_alloc(), PacketDeleter()), frame(av_frame_alloc(), FrameDeleter()) {

	int status;

	AVFormatContext* fmt_ctx = avformat_alloc_context();
	if (!fmt_ctx) 
		throw std::exception("Couldn't created AVFormatContext");


	if (status = avformat_open_input(&fmt_ctx, filename.c_str(), NULL, NULL) < 0)
		throw std::exception("cannot open input file");

	/*Socope for deleting fc imideatly*/
	{
		std::unique_ptr<AVFormatContext, FormatContextDeleter> fc{ fmt_ctx, FormatContextDeleter() };
		formatContext.swap(fc);

		fmt_ctx = nullptr;
	}
	
	if (status = avformat_find_stream_info(formatContext.get(), NULL) < 0)
		throw std::exception("cannot get stream info");

	/*find video stream*/
	VideoStreamIndex = FindVideoStream();
	if (VideoStreamIndex < 0) 
		throw std::exception("No video stream");

#ifdef _DEBUG
	//print detailed info for format
	av_dump_format(formatContext.get(), VideoStreamIndex, filename.c_str(), false);
#endif

	if (status = avcodec_parameters_to_context(codecContext.get(), formatContext->streams[VideoStreamIndex]->codecpar) < 0)
		throw std::exception("Cannot get codec parameters");

	Codec = avcodec_find_decoder(codecContext->codec_id);

	if (!Codec)
		throw std::exception("No decoder found");

	/**
	*
	* not needed if avcodec_alloc_context3() has non-Null argument
	* but in this case codec initialise with codecContext data, so it should initialie after codec
	* avcodec_open2() should have the same codec as pased to avcodec_alloc_context3()
	* if skip this this may couse problems with avcodec_send_packet()
	*/
	if (status = avcodec_open2(codecContext.get(), Codec, NULL) < 0)
		throw std::exception("Cannot open video decoder");

#ifdef _DEBUG
	std::cout << std::endl << "Decoding codec is : " << Codec->name << std::endl;
#endif 

	av_init_packet(packet.get());

	if (!packet)
		throw std::exception("Cannot init packet");

	if (!frame) 
		throw std::exception("Cannot init frame");
}

std::shared_ptr<AVFrame> VideoParcer::GetFrame()
{
	int status { 0 };
	while (true) {
		// read an encoded packet from file
		if (status = av_read_frame(formatContext.get(), packet.get()) < 0) {
			break;
		}

		if (packet->stream_index == VideoStreamIndex) {

			//send packet to decoder
			int ret = avcodec_send_packet(codecContext.get(), packet.get());
			if (ret < 0)
				throw std::exception("Error sending a packet for decoding");
			while (ret >= 0) {
				// receive frame from decoder
				// we may receive multiple frames or we may consume all data from decoder, then return to main loop
				ret = avcodec_receive_frame(codecContext.get(), frame.get());
				if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
					break;
				else if (ret < 0)
					throw std::exception("Error during decoding");

#ifdef _DEBUG
				std::cout << "saving frame " << codecContext->frame_number << std::endl;
#endif 
				return frame;
			}
		}
			av_packet_unref(packet.get());
	}

		/*send packet to decoder*/
		int ret = avcodec_send_packet(codecContext.get(), nullptr);
		//this may be usful, but not on case of class member function 
		/*if (ret < 0)
			throw std::exception("Error sending a packet for decoding");*/
		while (ret >= 0) {
			// receive frame from decoder
			// we may receive multiple frames or we may consume all data from decoder, then return to main loop
			ret = avcodec_receive_frame(codecContext.get(), frame.get());
			if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
				break;
			else if (ret < 0)
				throw std::exception("Error during decoding");

#ifdef _DEBUG
			std::cout << "saving frame " << codecContext->frame_number << std::endl;
#endif 
			return frame;

		}
		return nullptr;

}

const AVCodecContext* const VideoParcer::GetContext() const{
	return codecContext.get();
}

//bool VideoParcer::Decode(bool packetIsNULL)
//{
//	int ret;
//	AVPacket* pkt;
//	if (!packetIsNULL)
//		pkt = packet.get();
//	else
//		pkt = nullptr;
//
//	
//		return frame;
//	}
//	return nullptr;


int VideoParcer::FindVideoStream() {

	for (int i = 0; i < formatContext->nb_streams; i++) 
		if (formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) 
			return i;
}

