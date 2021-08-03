#include <iostream>
extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/avutil.h"
#include "libswscale/swscale.h"
}

using namespace std;
//decoder
int main()
{
	//初始化解封装
	av_register_all();
	//初始化网络库，（可以打开rtsp rtmp http协议的流媒体视频）
	avformat_network_init();

	//注册解码器
	avcodec_register_all();
	AVFormatContext *acContext = NULL;
	char * pPath = "E:\\ffmpeg_test_video\\test1.mp4";
	AVDictionary *oPts = NULL;
	//设置rtsp流以tcp协议打开
	av_dict_set(&oPts, "rtsp_transport", "tcp", 0);
	av_dict_set(&oPts, "max_delay", "500", 0);
	int ret = avformat_open_input(
		&acContext,
		pPath,
		0, //0表示自动选择解封装器
		&oPts); //参数设置，比如rtsp的延时时间
	if (ret != 0)
	{
		char buf[1024];
		av_strerror(ret, buf, sizeof(buf) - 1);
		cout << "open " << pPath << "failed : " << buf << endl;
		cout << ret << endl;
	}
	cout << "open success!" << endl;

	//获取流信息
	ret = avformat_find_stream_info(acContext, 0);
	//总时长 ms
	int nTotalTime = acContext->duration / (AV_TIME_BASE / 1000);
	cout << "total ms = " << nTotalTime << "ms" << endl;
	av_dump_format(acContext, 0, pPath, 0);

	//音视频索引，读取时区分音视频
	int eVideoStream = 0;
	int eAudioStream = 1;

	//获取音视频流信息（遍历，函数获取）
	for (int i = 0; i < acContext->nb_streams; i++)
	{
		AVStream *avStream = acContext->streams[i];
		if (avStream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			eAudioStream = i;
			cout << i << "音频信息" << endl;
			cout << "sampleRate: " << avStream->codecpar->sample_rate << endl;
			cout << "format = " << avStream->codecpar->format << endl;
			cout << "channels = " << avStream->codecpar->channels << endl;
			cout << "codec_id = " << avStream->codecpar->codec_id << endl;
			double nFps = avStream->avg_frame_rate.den == 0 ? 0 : (double)avStream->avg_frame_rate.num / (double)avStream->avg_frame_rate.den;
			cout << "nFps = " << nFps << endl;
			cout << "frameSize = " << avStream->codecpar->frame_size << endl;
		}
		
		else if(avStream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			eVideoStream = i;
			cout << i << "视频信息" << endl;
			cout << "width = " << avStream->codecpar->width << endl;
			cout << "height = " << avStream->codecpar->height << endl;
			cout << "codecId = " << avStream->codecpar->codec_id << endl;
			double nFps = avStream->avg_frame_rate.den == 0 ? 0 : (double)avStream->avg_frame_rate.num / (double)avStream->avg_frame_rate.den;
			cout << "nFps = " << nFps << endl;
		}
		else if (avStream->codecpar->codec_type == AVMEDIA_TYPE_SUBTITLE)
		{
			cout << i << "字幕信息" << endl;
		}
	}
	eVideoStream = av_find_best_stream(acContext, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0); //第二种获取视频流的方式
	

/************************视频解码器打开*****************************/
	//找到解码器.视频解码器
	AVCodec* pVideoCodec = avcodec_find_decoder(acContext->streams[eVideoStream]->codecpar->codec_id);
	if (!pVideoCodec)
	{
		cout << "can not find the codec id" << acContext->streams[eVideoStream]->codecpar->codec_id << endl;
		return -1;
	}
	//创建解码器上下文
	AVCodecContext *pVideoCtx = avcodec_alloc_context3(pVideoCodec);

	//配置上下文参数
	avcodec_parameters_to_context(pVideoCtx, acContext->streams[eVideoStream]->codecpar);

	//8线程解码
	pVideoCtx->thread_count = 8;

	//打开解码器上下文
	ret = avcodec_open2(pVideoCtx, 0, 0);
	if (ret != 0)
	{
		cout << "can not open video avcodec" << endl;
		return -1;
	}
	cout << "open video codec success" << endl;


/************************音频解码器打开*****************************/
	AVCodec *pAudioCodec = avcodec_find_decoder(acContext->streams[eAudioStream]->codecpar->codec_id);
	if (!pAudioCodec)
	{
		cout << "can not find the codec id" << acContext->streams[eAudioStream]->codecpar->codec_id << endl;
		return -1;
	}
	//创建解码器上下文
	AVCodecContext *pAudioCtx = avcodec_alloc_context3(pAudioCodec);

	//配置上下文参数
	avcodec_parameters_to_context(pAudioCtx, acContext->streams[eAudioStream]->codecpar);

	//8线程解码
	pAudioCtx->thread_count = 8;

	//打开解码器上下文

	ret = avcodec_open2(pAudioCtx, 0, 0);
	if (ret != 0)
	{
		cout << "can not open audio avcodec" << endl;
		return -1;
	}
	cout << "open audio codec success" << endl;


/************************解封装，并解码****************************/
	AVPacket *pPkt = av_packet_alloc();
	AVFrame *pFrame = av_frame_alloc();

	//像素格式和大小上下文
	SwsContext *pSctx = NULL;
	unsigned char* pRgb = NULL;
	for (;;)
	{
		int ret = av_read_frame(acContext, pPkt);
		if (ret != 0)
		{
			/*int ms = 3000;
			long long nPos = (double)ms / (double)1000 *
			av_seek_frame(acContext, )*/
			break;
		}
		//cout << pPkt->size << endl;
		//显示的时间
		//cout << "pPkt->pts" << pPkt->pts << endl;
		//解码时间
		//cout << "pPkt->dts" << pPkt->dts << endl;
		AVCodecContext *pDecodeCtx = NULL;
		if (pPkt->stream_index == eVideoStream)
		{
			//cout << "picture" << endl;
			pDecodeCtx = pVideoCtx;
		}
		if (pPkt->stream_index == eAudioStream)
		{
			pDecodeCtx = pAudioCtx;
		}

		//解码视频
		//发送packet到解码线程  send传NULL后调用多次recive取出所有缓冲帧
		ret = avcodec_send_packet(pDecodeCtx, pPkt);
		//释放，引用计数-1 为0释放空间
		av_packet_unref(pPkt);
		if (ret != 0)
		{
			char buf[1024] = { 0 };
			av_strerror(ret, buf, sizeof(buf) - 1);
			cout << "avcodec_send_packet failed " << buf << endl;
			break;
		}
		for (;;)
		{
			//不占用cpu, 从线程中获取解码接口，一次send可能对应多次recive
			ret = avcodec_receive_frame(pDecodeCtx, pFrame);
			if (ret != 0)
			{
				break;
			}
			cout << "recv frame " << pFrame->format
				<< "." << pFrame->linesize[0] << endl;
			if (pDecodeCtx == pVideoCtx)
			{
				pSctx = sws_getCachedContext(
					pSctx, //传NULL会新创建
					pFrame->width, pFrame->height, //输入宽高
					(AVPixelFormat)pFrame->format, //输入格式
					pFrame->width, pFrame->height, //输出宽高
					AV_PIX_FMT_RGBA, //输出格式
					SWS_BILINEAR, //尺寸变换的算法
					0,0,0);
				if (pSctx)
				{
					cout << "像素格式尺寸上下文创建成功" << endl;
				}
				else
				{
					cout << "像素格式尺寸上下文创建失败" << endl;
				}
				if (pSctx)
				{
					if (!pRgb)
					{
						pRgb = new unsigned char[pFrame->width * pFrame->height * 4];
					}
					uint8_t *pData[2] = { 0 };
					pData[0] = pRgb;
					int lines[2] = { 0 };
					lines[0] = pFrame->width * 4;
					sws_scale(pSctx,
						pFrame->data,
						pFrame->linesize,
						0,
						pFrame->height,
						pData,
						lines
						);
					cout << "sws_scale = " << ret << endl;
				}
			}
		}
	}
	if (pRgb)
	{
		delete [] pRgb;
		pRgb = NULL;
	}
	av_frame_free(&pFrame);
	av_packet_free(&pPkt);
	if (acContext)
	{
		//释放封装上下文，并把上下文置空
		avformat_close_input(&acContext);
	}
	return 0;
}