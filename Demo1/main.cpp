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
	//��ʼ�����װ
	av_register_all();
	//��ʼ������⣬�����Դ�rtsp rtmp httpЭ�����ý����Ƶ��
	avformat_network_init();

	//ע�������
	avcodec_register_all();
	AVFormatContext *acContext = NULL;
	char * pPath = "E:\\ffmpeg_test_video\\test1.mp4";
	AVDictionary *oPts = NULL;
	//����rtsp����tcpЭ���
	av_dict_set(&oPts, "rtsp_transport", "tcp", 0);
	av_dict_set(&oPts, "max_delay", "500", 0);
	int ret = avformat_open_input(
		&acContext,
		pPath,
		0, //0��ʾ�Զ�ѡ����װ��
		&oPts); //�������ã�����rtsp����ʱʱ��
	if (ret != 0)
	{
		char buf[1024];
		av_strerror(ret, buf, sizeof(buf) - 1);
		cout << "open " << pPath << "failed : " << buf << endl;
		cout << ret << endl;
	}
	cout << "open success!" << endl;

	//��ȡ����Ϣ
	ret = avformat_find_stream_info(acContext, 0);
	//��ʱ�� ms
	int nTotalTime = acContext->duration / (AV_TIME_BASE / 1000);
	cout << "total ms = " << nTotalTime << "ms" << endl;
	av_dump_format(acContext, 0, pPath, 0);

	//����Ƶ��������ȡʱ��������Ƶ
	int eVideoStream = 0;
	int eAudioStream = 1;

	//��ȡ����Ƶ����Ϣ��������������ȡ��
	for (int i = 0; i < acContext->nb_streams; i++)
	{
		AVStream *avStream = acContext->streams[i];
		if (avStream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			eAudioStream = i;
			cout << i << "��Ƶ��Ϣ" << endl;
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
			cout << i << "��Ƶ��Ϣ" << endl;
			cout << "width = " << avStream->codecpar->width << endl;
			cout << "height = " << avStream->codecpar->height << endl;
			cout << "codecId = " << avStream->codecpar->codec_id << endl;
			double nFps = avStream->avg_frame_rate.den == 0 ? 0 : (double)avStream->avg_frame_rate.num / (double)avStream->avg_frame_rate.den;
			cout << "nFps = " << nFps << endl;
		}
		else if (avStream->codecpar->codec_type == AVMEDIA_TYPE_SUBTITLE)
		{
			cout << i << "��Ļ��Ϣ" << endl;
		}
	}
	eVideoStream = av_find_best_stream(acContext, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0); //�ڶ��ֻ�ȡ��Ƶ���ķ�ʽ
	

/************************��Ƶ��������*****************************/
	//�ҵ�������.��Ƶ������
	AVCodec* pVideoCodec = avcodec_find_decoder(acContext->streams[eVideoStream]->codecpar->codec_id);
	if (!pVideoCodec)
	{
		cout << "can not find the codec id" << acContext->streams[eVideoStream]->codecpar->codec_id << endl;
		return -1;
	}
	//����������������
	AVCodecContext *pVideoCtx = avcodec_alloc_context3(pVideoCodec);

	//���������Ĳ���
	avcodec_parameters_to_context(pVideoCtx, acContext->streams[eVideoStream]->codecpar);

	//8�߳̽���
	pVideoCtx->thread_count = 8;

	//�򿪽�����������
	ret = avcodec_open2(pVideoCtx, 0, 0);
	if (ret != 0)
	{
		cout << "can not open video avcodec" << endl;
		return -1;
	}
	cout << "open video codec success" << endl;


/************************��Ƶ��������*****************************/
	AVCodec *pAudioCodec = avcodec_find_decoder(acContext->streams[eAudioStream]->codecpar->codec_id);
	if (!pAudioCodec)
	{
		cout << "can not find the codec id" << acContext->streams[eAudioStream]->codecpar->codec_id << endl;
		return -1;
	}
	//����������������
	AVCodecContext *pAudioCtx = avcodec_alloc_context3(pAudioCodec);

	//���������Ĳ���
	avcodec_parameters_to_context(pAudioCtx, acContext->streams[eAudioStream]->codecpar);

	//8�߳̽���
	pAudioCtx->thread_count = 8;

	//�򿪽�����������

	ret = avcodec_open2(pAudioCtx, 0, 0);
	if (ret != 0)
	{
		cout << "can not open audio avcodec" << endl;
		return -1;
	}
	cout << "open audio codec success" << endl;


/************************���װ��������****************************/
	AVPacket *pPkt = av_packet_alloc();
	AVFrame *pFrame = av_frame_alloc();

	//���ظ�ʽ�ʹ�С������
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
		//��ʾ��ʱ��
		//cout << "pPkt->pts" << pPkt->pts << endl;
		//����ʱ��
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

		//������Ƶ
		//����packet�������߳�  send��NULL����ö��reciveȡ�����л���֡
		ret = avcodec_send_packet(pDecodeCtx, pPkt);
		//�ͷţ����ü���-1 Ϊ0�ͷſռ�
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
			//��ռ��cpu, ���߳��л�ȡ����ӿڣ�һ��send���ܶ�Ӧ���recive
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
					pSctx, //��NULL���´���
					pFrame->width, pFrame->height, //������
					(AVPixelFormat)pFrame->format, //�����ʽ
					pFrame->width, pFrame->height, //������
					AV_PIX_FMT_RGBA, //�����ʽ
					SWS_BILINEAR, //�ߴ�任���㷨
					0,0,0);
				if (pSctx)
				{
					cout << "���ظ�ʽ�ߴ������Ĵ����ɹ�" << endl;
				}
				else
				{
					cout << "���ظ�ʽ�ߴ������Ĵ���ʧ��" << endl;
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
		//�ͷŷ�װ�����ģ������������ÿ�
		avformat_close_input(&acContext);
	}
	return 0;
}