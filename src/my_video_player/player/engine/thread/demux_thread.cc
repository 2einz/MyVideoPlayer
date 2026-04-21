#include "player/engine/thread/demux_thread.h"

#include <iostream>

#include "player/engine/demux/demuxer.h"
#include "player/engine/buffer/video_packet_queue.h"
#include "player/media_state.hpp"

namespace my_video_player {
DemuxThread::~DemuxThread() {
    Stop();
}

void DemuxThread::Start(
    Demuxer* demuxer, VideoPacketQueue* pkt_queue, MediaState* state, std::atomic<int>* serial, FinishCallback cb) {
    demuxer_ = demuxer;
    pkt_queue_ = pkt_queue;
    media_state_ = state;
    serial_ = serial;
    on_finished_ = std::move(cb);

    running_ = true;
    thread_ = std::thread(&DemuxThread::Run, this);
}

void DemuxThread::Stop() {
    if (!running_)
        return;

    running_ = false;

    // 注意：如果线程正阻塞在 pkt_queue_->Push() 中，
    // 外部 (Player::Stop) 必须先调用 pkt_queue_->Abort() 唤醒它，
    // 否则这里的 join 会死锁。
    if (thread_.joinable()) {
        thread_.join();
    }
}

void DemuxThread::Run() {
    PacketItem pkt;

    while (running_) {
        if (!pkt.packet) {
            pkt.packet = MakeAvPacketPtr();
        }

        // 读包
        int ret = demuxer_->ReadPacket(&pkt);
        if (ret < 0) {
            // 读到文件尾 或是 IO错误: 使用Close，允许解码线程把队列剩下的包读完
            pkt_queue_->Close();
            if (on_finished_) {
                on_finished_();
            }
            std::cout << "[DemuxThread] Reached end of file or error." << std::endl;
            break;
        }

        // 分发与盖章
        if (pkt.stream_index == demuxer_->video_stream_index()) {
            // 盖上全局序列号
            pkt.serial = serial_->load();

            // 压入队列
            if (!pkt_queue_->Push(std::move(pkt))) {
                break;
            }
        }
    }
    std::cout << "[DemuxThread] Thread exiting..." << std::endl;
}

} // namespace my_video_player
