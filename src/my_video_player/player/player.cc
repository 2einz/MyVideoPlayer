#include "player/player.h"

#include <iostream>

namespace my_video_player {

Player::Player() {}

int Player::Open(const std::string& url) {
    std::cout << "[Player] Attempting to open: " << url << std::endl;

    // 打开文件
    int ret = demuxer_.Open(url);
    if (ret < 0) {
        std::cerr << "[Player] Demuxer open failed: " << ret << std::endl;
        return -1;
    }
    std::cout << "[Player] Demuxer opened successfully." << std::endl;

    // 初始化视频解码器
    AVCodecParameters* v_params = demuxer_.GetVideoCodecParameters();
    if (!v_params) {
        std::cerr << "[Player] Could not find video codec parameters." << std::endl;
        return -2; // 报错返回 -2
    }
    std::cout << "[Player] Found video stream, codec_id: " << v_params->codec_id << std::endl;

    ret = video_decoder_.Init(v_params);
    if (ret < 0) {
        std::cerr << "[Player] Video decoder init failed: " << ret << std::endl;
        return -3; // 报错返回 -3
    }
    std::cout << "[Player] Video decoder initialized." << std::endl;

    // 测试解码
    PacketItem pkt_item;
    FrameItem frame_item;

    AVRational v_time_base = demuxer_.format_context()->streams[demuxer_.video_stream_index()]->time_base;

    int count = 0;
    while (count < 10) { // 测试解 10 帧
        int read_ret = demuxer_.ReadPacket(&pkt_item);
        if (read_ret < 0) {
            std::cerr << "[Player] Read packet failed during test: " << read_ret << std::endl;
            break;
        }
        if (pkt_item.stream_index == demuxer_.video_stream_index()) {
            if (video_decoder_.SendPacket(pkt_item) == 0) {
                while (video_decoder_.ReceiveFrame(&frame_item, v_time_base) == 0) {
                    count++;
                    std::printf("[Player Test] Decoded Frame %d | PTS: %.3f\n", count, frame_item.pts);
                }
            }
        }
    }

    return 0;
}
} // namespace my_video_player