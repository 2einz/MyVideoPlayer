#include "player/engine/decode/video_decoder.h"
#include "player/engine/common/error_util.h"

#include <iostream>

extern "C" {
#include <libavcodec/avcodec.h>
}

namespace my_video_player {

int VideoDecoder::Init(AVCodecParameters* params) {
    // 查找解码器
    const AVCodec* codec = avcodec_find_decoder(params->codec_id);

    if (!codec) {
        std::cerr << "Cannot find decoder for ID: " << params->codec_id << std::endl;
        return AVERROR(EINVAL);
    }

    // 分配上下文
    codec_ctx_ = AvCodecContextPtr(avcodec_alloc_context3(codec));

    // 将参数拷贝到上下文
    if (avcodec_parameters_to_context(codec_ctx_.get(), params) < 0) {
        return AVERROR(EIO);
    }

    // 开启多线程解码（利用现代 CPU 多核性能）
    codec_ctx_->thread_count = 0; // 0 表示自动选择线程数

    // 打开解码器
    int ret = avcodec_open2(codec_ctx_.get(), codec, nullptr);
    if (ret < 0)
        return ret;

    std::cout << "Decoder initialized: " << codec->long_name << std::endl;
    return 0;
}

int VideoDecoder::SendPacket(const PacketItem& packet_item) {
    if (!codec_ctx_ || !packet_item.packet)
        return AVERROR(EINVAL);
    return avcodec_send_packet(codec_ctx_.get(), packet_item.packet.get());
}

int VideoDecoder::ReceiveFrame(FrameItem* frame_item, AVRational time_base) {
    if (!frame_item || !codec_ctx_)
        return AVERROR(EINVAL);

    // 复用或分配内存
    if (!frame_item->frame) {
        frame_item->frame = MakeAvFramePtr();
    } else {
        ResetFrame(frame_item->frame);
    }

    int ret = avcodec_receive_frame(codec_ctx_.get(), frame_item->frame.get());
    if (ret < 0)
        return ret;

    // 使用传入的流 time_base 计算该帧应该在第几秒显示
    // av_q2d 将分数转换为 double
    frame_item->pts = frame_item->frame->best_effort_timestamp * av_q2d(time_base);

    return 0;
}

} // namespace my_video_player