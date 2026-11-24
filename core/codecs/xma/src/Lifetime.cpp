#include <anybox/codecs/XmaFrameDecoder.hpp>
#include "FFmpeg.hpp"
#include "Logging.hpp"

void AnyBox::Codecs::XmaFrameDecoder::ContextDeleter::operator()(AVCodecContext* value) const noexcept { avcodec_free_context(&value); }
void AnyBox::Codecs::XmaFrameDecoder::FrameDeleter::operator()(AVFrame* value) const noexcept { av_frame_free(&value); }
void AnyBox::Codecs::XmaFrameDecoder::PacketDeleter::operator()(AVPacket* value) const noexcept { av_packet_free(&value); }

AnyBox::Codecs::XmaFrameDecoder::XmaFrameDecoder(std::uint32_t sampleRate, std::uint32_t channels) : sampleRate(sampleRate), channels(channels) {
    if (channels != 1 && channels != 2) throw std::invalid_argument("XMA requires one or two channels per stream");
    if (sampleRate != 24000 && sampleRate != 32000 && sampleRate != 44100 && sampleRate != 48000) throw std::invalid_argument("Invalid XMA sample rate");
    context = createContext();
    frame.reset(av_frame_alloc());
    packet.reset(av_packet_alloc());
    if (!frame || !packet) throw std::bad_alloc();
    CheckXma(av_new_packet(packet.get(), 4098), "Allocate XMA frame packet");
}

AnyBox::Codecs::XmaFrameDecoder::~XmaFrameDecoder() = default;

AnyBox::Codecs::XmaFrameDecoder::Context AnyBox::Codecs::XmaFrameDecoder::createContext() const {
    InitializeXmaLogging();
    const auto* codec = avcodec_find_decoder(AV_CODEC_ID_XMAFRAMES);
    if (!codec) throw std::runtime_error("XMA frame decoder is unavailable");
    Context result(avcodec_alloc_context3(codec));
    if (!result) throw std::bad_alloc();
    result->sample_rate = static_cast<int>(sampleRate);
    result->channels = static_cast<int>(channels);
    result->channel_layout = static_cast<std::uint64_t>(av_get_default_channel_layout(static_cast<int>(channels)));
    result->thread_count = 1;
    CheckXma(avcodec_open2(result.get(), codec, nullptr), "Initialize XMA frame decoder");
    return result;
}

void AnyBox::Codecs::XmaFrameDecoder::Reset() {
    auto replacement = createContext();
    av_frame_unref(frame.get());
    context = std::move(replacement);
    needsDrain = false;
    failed = false;
}
