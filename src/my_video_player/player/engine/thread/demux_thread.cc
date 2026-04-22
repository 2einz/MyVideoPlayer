#include "player/engine/thread/demux_thread.h"

#include <iostream>

#include "player/engine/demux/demuxer.h"
#include "player/engine/buffer/video_packet_queue.h"
#include "player/media_state.hpp"

namespace my_video_player {

void DemuxThread::Start(
    Demuxer* demuxer, VideoPacketQueue* pkt_queue, MediaState* state, std::atomic<int>* serial, FinishCallback cb) {
    demuxer_ = demuxer;
    pkt_queue_ = pkt_queue;
    media_state_ = state;
    serial_ = serial;
    on_finished_ = std::move(cb);

    // 调用基类的启动
    this->start();
}

void DemuxThread::stop() {
    if (pkt_queue_) {
        pkt_queue_->abort();
    }
    MyThread::stop();
}

void DemuxThread::run() {
    while (running_) {
        if (media_state_->IsStopped()) {
            LOG_INFO(LM::kThread, "DemuxThread: Stopped by media_state_");
            break;
        }

        PacketItem pkt;
        pkt.packet = MakeAvPacketPtr();

        // 读包
        int ret = demuxer_->ReadPacket(&pkt);
        if (ret == AVERROR_EOF) {
            pkt_queue_->set_eof();

            if (on_finished_) {
                on_finished_();
            }

            LOG_WARN(LM::kThread, "Reached end of file or error.");
            break;
        }

        if (ret < 0) {
            LOG_ERROR(LM::kThread, "Read error");
            break;
        }

        // 分发与盖章
        if (pkt.stream_index == demuxer_->video_stream_index()) {
            // 盖上全局序列号
            pkt.serial = serial_->load();

            // 压入队列
            if (!pkt_queue_->push(std::move(pkt))) {
                break;
            }
        }
    }
    LOG_INFO(LM::kThread, "DemuxThread is exiting...");
}

} // namespace my_video_player
