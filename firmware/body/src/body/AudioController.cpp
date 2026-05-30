#include "body/AudioController.hpp"

#include <M5StackChan.h>

namespace stackchan {
namespace {

uint16_t readLe16(const uint8_t* data)
{
    return static_cast<uint16_t>(data[0]) | (static_cast<uint16_t>(data[1]) << 8);
}

uint32_t readLe32(const uint8_t* data)
{
    return static_cast<uint32_t>(data[0])
        | (static_cast<uint32_t>(data[1]) << 8)
        | (static_cast<uint32_t>(data[2]) << 16)
        | (static_cast<uint32_t>(data[3]) << 24);
}

}  // namespace

void AudioController::begin()
{
    mode_ = AudioMode::Idle;
    auto config = M5.Speaker.config();
    config.dma_buf_len = 1024;
    config.dma_buf_count = 8;
    M5.Speaker.config(config);
    M5.Speaker.setVolume(state_.volume());
    M5.Speaker.setAllChannelVolume(255);
    state_.setState(AudioPlaybackState::Idle);
    state_.setPlaying(false);
    state_.setQueued(false);
    expectedDurationMs_ = 0;
    wavSampleRate_ = 0;
    parsedPcmOffset_ = 0;
    parsedPcmBytes_ = 0;
    Serial.println("[AUDIO] begin done (lazy init)");
}

void AudioController::update()
{
    if (mode_ == AudioMode::Playback && state_.isQueued()) {
        return;
    }

    if (state_.isPlaying()) {
        const bool speakerStillPlaying = M5.Speaker.isPlaying();
        const uint32_t elapsed = millis() - state_.startedAt();
        const bool timeExpired = (expectedDurationMs_ > 0) && (elapsed > expectedDurationMs_ + 500);

        if (!speakerStillPlaying || timeExpired) {
            if (timeExpired) {
                Serial.println("[AUDIO] failsafe: forcing stop after timeout");
                M5.Speaker.stop();
            }
            state_.setPlaying(false);
            state_.setState(AudioPlaybackState::Finished);
            state_.markFinished(millis());
            parsedPcmOffset_ = 0;
            parsedPcmBytes_ = 0;
            releaseBuffer();
        }
    }
}

AudioMode AudioController::mode() const
{
    return mode_;
}

void AudioController::setMode(AudioMode mode)
{
    mode_ = mode;
}

bool AudioController::prepareWav(size_t size, String& error)
{
    if (state_.isPlaying()) {
        error = "AUDIO_BUSY";
        return false;
    }
    if (size == 0 || size > kMaxWavBytes) {
        error = "AUDIO_TOO_LARGE";
        return false;
    }

    releaseBuffer();
    wavBuffer_ = static_cast<uint8_t*>(ps_malloc(size));
    if (wavBuffer_ == nullptr) {
        wavBuffer_ = static_cast<uint8_t*>(malloc(size));
    }
    if (wavBuffer_ == nullptr) {
        error = "INTERNAL_ERROR";
        state_.setState(AudioPlaybackState::Error);
        state_.setError(error);
        return false;
    }

    state_.setState(AudioPlaybackState::Receiving);
    state_.setCurrentSize(size);
    state_.setReceivedSize(0);
    state_.clearError();
    return true;
}

uint8_t* AudioController::preparedBuffer()
{
    return wavBuffer_;
}

bool AudioController::parseWavPcm16Mono(const uint8_t* buffer, size_t size, ParsedWav& out) const
{
    out = ParsedWav{};

    if (buffer == nullptr || size < 44) {
        return false;
    }
    if (memcmp(buffer, "RIFF", 4) != 0 || memcmp(buffer + 8, "WAVE", 4) != 0) {
        return false;
    }

    size_t offset = 12;
    bool foundFmt = false;
    bool foundData = false;
    uint16_t audioFormat = 0;

    while (offset + 8 <= size) {
        const uint8_t* chunk = buffer + offset;
        const uint32_t chunkSize = readLe32(chunk + 4);
        const size_t dataOffset = offset + 8;
        if (dataOffset + chunkSize > size) {
            return false;
        }

        if (memcmp(chunk, "fmt ", 4) == 0) {
            if (chunkSize < 16) {
                return false;
            }
            audioFormat = readLe16(buffer + dataOffset);
            out.bitsPerSample = readLe16(buffer + dataOffset + 14);
            out.channels = readLe16(buffer + dataOffset + 2);
            out.sampleRate = readLe32(buffer + dataOffset + 4);
            foundFmt = true;
        } else if (memcmp(chunk, "data", 4) == 0) {
            out.dataOffset = dataOffset;
            out.dataSize = chunkSize;
            foundData = chunkSize > 0;
        }

        offset = dataOffset + chunkSize + (chunkSize % 2);
    }

    if (!foundFmt || !foundData) {
        return false;
    }
    if (audioFormat != 1) {
        return false;
    }
    if (out.bitsPerSample != 16) {
        return false;
    }
    if (out.channels != 1) {
        return false;
    }
    if (out.sampleRate == 0) {
        return false;
    }
    if (out.dataSize == 0 || out.dataSize % sizeof(int16_t) != 0) {
        return false;
    }
    if (out.dataOffset + out.dataSize > size) {
        return false;
    }

    return true;
}

bool AudioController::queuePlayWav(uint8_t* buffer, size_t size, String& error)
{
    if (buffer == nullptr || buffer != wavBuffer_ || size != state_.currentSize()) {
        error = "AUDIO_TRANSFER_FAILED";
        state_.setState(AudioPlaybackState::Error);
        state_.setError(error);
        return false;
    }
    state_.setReceivedSize(size);

    ParsedWav parsed;
    if (!parseWavPcm16Mono(buffer, size, parsed)) {
        error = "AUDIO_INVALID_FORMAT";
        state_.setState(AudioPlaybackState::Error);
        state_.setError(error);
        releaseBuffer();
        return false;
    }

    wavSampleRate_ = parsed.sampleRate;
    parsedPcmOffset_ = parsed.dataOffset;
    parsedPcmBytes_ = parsed.dataSize;
    expectedDurationMs_ = static_cast<uint32_t>((parsed.dataSize * 1000ULL) / (parsed.sampleRate * sizeof(int16_t)));

    state_.setState(AudioPlaybackState::Queued);
    state_.setQueued(true);
    return true;
}

int16_t AudioController::maxAbsFirstSamples(const int16_t* pcm, size_t samples, size_t maxCount) const
{
    if (pcm == nullptr || samples == 0) {
        return 0;
    }
    const size_t n = (samples < maxCount) ? samples : maxCount;
    int16_t maxAbs = 0;
    for (size_t i = 0; i < n; ++i) {
        const int16_t v = (pcm[i] >= 0) ? pcm[i] : static_cast<int16_t>(-pcm[i]);
        if (v > maxAbs) {
            maxAbs = v;
        }
    }
    return maxAbs;
}

bool AudioController::startQueuedPlay(String& error)
{
    if (!state_.isQueued()) {
        error = "AUDIO_NOT_QUEUED";
        return false;
    }

    if (wavBuffer_ == nullptr) {
        error = "AUDIO_BUFFER_LOST";
        state_.setState(AudioPlaybackState::Error);
        state_.setError(error);
        state_.setQueued(false);
        return false;
    }

    if (parsedPcmBytes_ == 0 || wavSampleRate_ == 0) {
        error = "AUDIO_INVALID_PCM";
        state_.setState(AudioPlaybackState::Error);
        state_.setError(error);
        state_.setQueued(false);
        releaseBuffer();
        return false;
    }

    const int16_t* pcm = reinterpret_cast<const int16_t*>(wavBuffer_ + parsedPcmOffset_);
    const size_t samples = parsedPcmBytes_ / sizeof(int16_t);
    const int16_t maxAbs = maxAbsFirstSamples(pcm, samples, 512);

    Serial.printf(
        "[AUDIO] playRaw PCM: rate=%u offset=%u bytes=%u samples=%u first=%d maxAbs=%d\n",
        wavSampleRate_,
        static_cast<unsigned>(parsedPcmOffset_),
        static_cast<unsigned>(parsedPcmBytes_),
        static_cast<unsigned>(samples),
        samples > 0 ? pcm[0] : 0,
        maxAbs
    );

    logHardwareState("before playRaw");

    const bool ok = M5.Speaker.playRaw(
        pcm,
        samples,
        wavSampleRate_,
        false,
        1,
        0,
        true
    );

    Serial.printf(
        "[AUDIO] after playRaw: ok=%d spkPlaying=%d channels=%u\n",
        ok,
        M5.Speaker.isPlaying(),
        M5.Speaker.getPlayingChannels()
    );

    if (!ok) {
        error = "AUDIO_PLAY_RAW_FAILED";
        state_.setPlaying(false);
        state_.setQueued(false);
        state_.setState(AudioPlaybackState::Error);
        state_.setError(error);
        releaseBuffer();
        return false;
    }

    state_.setState(AudioPlaybackState::Playing);
    state_.setPlaying(true);
    state_.setQueued(false);
    state_.markStarted(millis());
    state_.clearError();
    setMode(AudioMode::Playback);

    logHardwareState("after playRaw");
    return true;
}

bool AudioController::beginSpeaker()
{
    Serial.println("[AUDIO] beginSpeaker: stop + config + begin");

    if (M5.Speaker.isPlaying()) {
        Serial.println("[AUDIO] speaker still playing, stopping first");
        M5.Speaker.stop();
        delay(30);
    }

    auto config = M5.Speaker.config();
    config.sample_rate = 48000;
    config.dma_buf_len = 1024;
    config.dma_buf_count = 8;
    M5.Speaker.config(config);

    const bool ok = M5.Speaker.begin();

    M5.Speaker.setVolume(state_.volume());
    M5.Speaker.setAllChannelVolume(255);

    logHardwareState("after beginSpeaker (config + begin)");
    return ok && M5.Speaker.isEnabled() && M5.Speaker.isRunning();
}

void AudioController::logHardwareState(const char* label) const
{
    Serial.printf(
        "[AUDIO] %s: spk enabled=%d running=%d playing=%d | mic enabled=%d running=%d recording=%d\n",
        label,
        M5.Speaker.isEnabled(),
        M5.Speaker.isRunning(),
        M5.Speaker.isPlaying(),
        M5.Mic.isEnabled(),
        M5.Mic.isRunning(),
        M5.Mic.isRecording()
    );
}

void AudioController::stop()
{
    M5.Speaker.stop();
    state_.setPlaying(false);
    state_.setQueued(false);
    state_.setState(AudioPlaybackState::Idle);
    state_.setCurrentSize(0);
    state_.setReceivedSize(0);
    expectedDurationMs_ = 0;
    wavSampleRate_ = 0;
    parsedPcmOffset_ = 0;
    parsedPcmBytes_ = 0;
    state_.markFinished(millis());
    releaseBuffer();
    setMode(AudioMode::Idle);
}

bool AudioController::setVolume(int volume)
{
    if (volume < 0 || volume > 255) {
        return false;
    }
    state_.setVolume(static_cast<uint8_t>(volume));
    M5.Speaker.setVolume(state_.volume());
    return true;
}

bool AudioController::isPlaying() const
{
    return state_.isPlaying();
}

const AudioState& AudioController::getState() const
{
    return state_;
}

void AudioController::releaseBuffer()
{
    if (wavBuffer_ != nullptr && !state_.isPlaying()) {
        free(wavBuffer_);
        wavBuffer_ = nullptr;
    }
}

}  // namespace stackchan
