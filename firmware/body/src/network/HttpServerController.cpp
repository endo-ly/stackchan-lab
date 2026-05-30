#include "network/HttpServerController.hpp"

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFiClient.h>

#include "body/AudioController.hpp"
#include "protocol/ProtocolTypes.hpp"

namespace stackchan::network {
namespace {

String lower(const String& value)
{
    String result = value;
    result.toLowerCase();
    return result;
}

bool parseExpressionValue(const String& value, Expression& expression)
{
    const String v = lower(value);
    if (v == "neutral") expression = Expression::Neutral;
    else if (v == "happy") expression = Expression::Happy;
    else if (v == "sad") expression = Expression::Sad;
    else if (v == "angry") expression = Expression::Angry;
    else if (v == "sleepy") expression = Expression::Sleepy;
    else if (v == "doubt") expression = Expression::Doubt;
    else return false;
    return true;
}

bool parseMoodValue(const String& value, Mood& mood)
{
    const String v = lower(value);
    if (v == "calm") mood = Mood::Calm;
    else if (v == "active") mood = Mood::Active;
    else if (v == "speaking") mood = Mood::Speaking;
    else if (v == "warning") mood = Mood::Warning;
    else if (v == "off") mood = Mood::Off;
    else return false;
    return true;
}

bool parsePoseValue(const String& value, MotionPose& pose)
{
    const String v = lower(value);
    if (v == "neutral") pose = MotionPose::Neutral;
    else if (v == "look_left") pose = MotionPose::LookLeft;
    else if (v == "look_right") pose = MotionPose::LookRight;
    else if (v == "look_up") pose = MotionPose::LookUp;
    else if (v == "look_down") pose = MotionPose::LookDown;
    else return false;
    return true;
}

void addEvent(JsonArray array, const InputEvent& event)
{
    JsonObject item = array.add<JsonObject>();
    item["id"] = event.id;
    item["type"] = event.type;
    item["target"] = event.target;
    item["value"] = event.value;
    item["timestamp"] = event.timestamp;
    if (event.x >= 0 && event.y >= 0) {
        item["x"] = event.x;
        item["y"] = event.y;
    }
}

bool postMultipartWav(const String& url, const String& source, const uint8_t* wav, size_t wavSize, int& statusCode, String& response, String& error)
{
    if (wav == nullptr || wavSize == 0) {
        error = "MIC_WAV_EMPTY";
        return false;
    }

    const String boundary = "----stackchan-mic-boundary";
    const String part1 = String("--") + boundary + "\r\n"
        + "Content-Disposition: form-data; name=\"source\"\r\n\r\n"
        + source + "\r\n"
        + "--" + boundary + "\r\n"
        + "Content-Disposition: form-data; name=\"file\"; filename=\"stackchan.wav\"\r\n"
        + "Content-Type: audio/wav\r\n\r\n";
    const String part2 = String("\r\n--") + boundary + "--\r\n";
    const size_t bodySize = part1.length() + wavSize + part2.length();
    uint8_t* body = static_cast<uint8_t*>(ps_malloc(bodySize));
    if (body == nullptr) {
        body = static_cast<uint8_t*>(malloc(bodySize));
    }
    if (body == nullptr) {
        error = "MIC_UPLOAD_BUFFER_ALLOC_FAILED";
        return false;
    }

    size_t offset = 0;
    memcpy(body + offset, part1.c_str(), part1.length());
    offset += part1.length();
    memcpy(body + offset, wav, wavSize);
    offset += wavSize;
    memcpy(body + offset, part2.c_str(), part2.length());

    WiFiClient client;
    HTTPClient http;
    if (!http.begin(client, url)) {
        free(body);
        error = "MIC_UPLOAD_BEGIN_FAILED";
        return false;
    }

    http.setTimeout(45000);
    http.addHeader("Content-Type", String("multipart/form-data; boundary=") + boundary);
    statusCode = http.POST(body, bodySize);
    response = http.getString();
    http.end();
    free(body);

    if (statusCode < 200 || statusCode >= 300) {
        error = String("MIC_UPLOAD_HTTP_") + statusCode;
        return false;
    }
    return true;
}

}  // namespace

HttpServerController::HttpServerController(BodyController& body, WiFiManager& wifi, DeviceAuth& auth)
    : body_(body)
    , wifi_(wifi)
    , auth_(auth)
    , server_(80)
{
}

void HttpServerController::begin()
{
    if (running_) {
        return;
    }
    static const char* headerKeys[] = {"X-StackChan-Token"};
    server_.collectHeaders(headerKeys, 1);
    registerRoutes();
    server_.begin();
    running_ = true;
    Serial.println("[HTTP] server started");
}

void HttpServerController::update()
{
    if (running_) {
        server_.handleClient();
        processWakeDetection();
    }
}

bool HttpServerController::isRunning() const
{
    return running_;
}

void HttpServerController::registerRoutes()
{
    server_.on("/health", HTTP_GET, [this]() { handleHealth(); });
    server_.on("/version", HTTP_GET, [this]() { handleVersion(); });
    server_.on("/status", HTTP_GET, [this]() { handleStatus(); });
    server_.on("/capabilities", HTTP_GET, [this]() { handleCapabilities(); });
    server_.on("/face", HTTP_POST, [this]() { handleFace(); });
    server_.on("/led", HTTP_POST, [this]() { handleLed(); });
    server_.on("/pose", HTTP_POST, [this]() { handlePose(); });
    server_.on("/move", HTTP_POST, [this]() { handleMove(); });
    server_.on("/reset", HTTP_POST, [this]() { handleReset(); });
    server_.on("/play-wav", HTTP_POST, [this]() { handlePlayWav(); }, [this]() { handlePlayWavBody(); });
    server_.on("/audio/status", HTTP_GET, [this]() { handleAudioStatus(); });
    server_.on("/audio/volume", HTTP_POST, [this]() { handleAudioVolume(); });
    server_.on("/audio/stop", HTTP_POST, [this]() { handleAudioStop(); });
    server_.on("/mic/status", HTTP_GET, [this]() { handleMicStatus(); });
    server_.on("/mic/record", HTTP_POST, [this]() { handleMicRecord(); });
    server_.on("/wake/status", HTTP_GET, [this]() { handleWakeStatus(); });
    server_.on("/wake/start", HTTP_POST, [this]() { handleWakeStart(); });
    server_.on("/wake/stop", HTTP_POST, [this]() { handleWakeStop(); });
    server_.on("/events", HTTP_GET, [this]() { handleEvents(); });
    server_.on("/events/latest", HTTP_GET, [this]() { handleLatestEvent(); });
    server_.on("/events/clear", HTTP_POST, [this]() { handleClearEvents(); });
    server_.on("/wifi/status", HTTP_GET, [this]() { handleWifiStatus(); });
    server_.on("/wifi/connect", HTTP_POST, [this]() { handleWifiConnect(); });
    server_.on("/wifi/clear", HTTP_POST, [this]() { handleWifiClear(); });
    server_.on("/camera/snapshot", HTTP_GET, [this]() { handleCameraSnapshot(); });
    server_.onNotFound([this]() { sendError(404, "NOT_FOUND", "Route not found"); });
}

void HttpServerController::processWakeDetection()
{
    if (wakeUploadInProgress_) {
        return;
    }

    const WakeState& wake = body_.getWakeState();
    if (!wake.detected() || wake.lastDetectedAtMs() == 0 || wake.lastDetectedAtMs() == handledWakeDetectedAtMs_) {
        return;
    }

    handledWakeDetectedAtMs_ = wake.lastDetectedAtMs();
    lastWakeUploadAtMs_ = millis();
    lastWakeUploadHttpStatus_ = 0;
    lastWakeUploadError_ = "";
    lastWakeSpeechResponse_ = "";

    const String url = wifi_.config().speechServicesUrl;
    if (url.length() == 0) {
        lastWakeUploadError_ = "SPEECH_SERVICES_NOT_CONFIGURED";
        restartWakeDetection();
        return;
    }

    wakeUploadInProgress_ = true;
    body_.showWakeDetected();
    String error;
    if (!body_.recordMicWav(3000, error)) {
        lastWakeUploadError_ = error;
        wakeUploadInProgress_ = false;
        restartWakeDetection();
        return;
    }

    int statusCode = 0;
    String response;
    const bool uploaded = postMultipartWav(url, "stackchan-wake", body_.micWavBuffer(), body_.micWavSize(), statusCode, response, error);
    lastWakeUploadHttpStatus_ = statusCode;
    lastWakeSpeechResponse_ = response;
    if (!uploaded) {
        lastWakeUploadError_ = error;
    }

    wakeUploadInProgress_ = false;
    restartWakeDetection();
}

void HttpServerController::restartWakeDetection()
{
    body_.clearWakeDetected();
    String wakeError;
    if (!body_.startWake(wakeError)) {
        lastWakeUploadError_ = lastWakeUploadError_.length() > 0 ? lastWakeUploadError_ : wakeError;
    }
}

bool HttpServerController::requireAuth()
{
    if (auth_.authorize(server_)) {
        return true;
    }
    sendError(401, "UNAUTHORIZED", "Invalid token");
    return false;
}

void HttpServerController::sendOk(const String& dataJson)
{
    server_.send(200, "application/json", String("{\"ok\":true,\"data\":") + dataJson + "}");
}

void HttpServerController::sendError(int status, const char* code, const String& message)
{
    JsonDocument doc;
    doc["ok"] = false;
    JsonObject error = doc["error"].to<JsonObject>();
    error["code"] = code;
    error["message"] = message;
    String response;
    serializeJson(doc, response);
    server_.send(status, "application/json", response);
}

String HttpServerController::readBody()
{
    return server_.arg("plain");
}

void HttpServerController::handleHealth()
{
    sendOk("{\"service\":\"stackchan-device\",\"status\":\"ok\"}");
}

void HttpServerController::handleVersion()
{
    sendOk(String("{\"firmware\":\"") + protocol::kFirmwareVersion + "\",\"protocol\":\"" + protocol::kProtocolVersion + "\",\"transport\":\"wifi\",\"board\":\"" + protocol::kBoardName + "\"}");
}

void HttpServerController::handleStatus()
{
    if (!requireAuth()) return;
    JsonDocument doc;
    const BodyState& state = body_.getState();
    const FaceState& face = body_.getFaceState();
    const InputState& input = body_.input().getState();
    doc["mode"] = toString(state.mode());
    doc["expression"] = toString(state.expression());
    doc["mood"] = toString(state.mood());
    doc["pose"] = toString(state.pose());
    doc["x"] = state.servoX();
    doc["y"] = state.servoY();
    doc["gazeX"] = face.gazeX();
    doc["gazeY"] = face.gazeY();
    doc["speaking"] = face.isSpeaking();
    doc["blinking"] = face.isBlinking();
    doc["eventCount"] = body_.input().eventCount();
    doc["latestEvent"] = input.latestType();
    doc["latestTarget"] = input.latestTarget();
    doc["latestValue"] = input.latestValue();
    doc["touchActive"] = input.touchActive();
    doc["buttonActive"] = input.buttonActive();
    doc["imuMoving"] = input.imuMoving();
    JsonObject network = doc["network"].to<JsonObject>();
    const NetworkState& net = wifi_.state();
    network["connected"] = net.connected;
    network["ip"] = net.ip;
    network["hostname"] = net.hostname;
    network["rssi"] = net.rssi;
    String data;
    serializeJson(doc, data);
    sendOk(data);
}

void HttpServerController::handleCapabilities()
{
    sendOk("{\"expressions\":[\"neutral\",\"happy\",\"sad\",\"angry\",\"sleepy\",\"doubt\"],\"moods\":[\"calm\",\"active\",\"speaking\",\"warning\",\"off\"],\"poses\":[\"neutral\",\"look_left\",\"look_right\",\"look_up\",\"look_down\"]}");
}

void HttpServerController::handleFace()
{
    if (!requireAuth()) return;
    JsonDocument doc;
    if (deserializeJson(doc, readBody())) return sendError(400, "INVALID_ARGUMENT", "Invalid JSON");
    Expression expression;
    if (!parseExpressionValue(doc["expression"] | "", expression)) return sendError(400, "INVALID_ARGUMENT", "Unsupported expression");
    body_.setExpression(expression);
    sendOk(String("{\"expression\":\"") + lower(doc["expression"].as<String>()) + "\"}");
}

void HttpServerController::handleLed()
{
    if (!requireAuth()) return;
    JsonDocument doc;
    if (deserializeJson(doc, readBody())) return sendError(400, "INVALID_ARGUMENT", "Invalid JSON");
    Mood mood;
    if (!parseMoodValue(doc["mood"] | "", mood)) return sendError(400, "INVALID_ARGUMENT", "Unsupported mood");
    body_.setMood(mood);
    sendOk(String("{\"mood\":\"") + lower(doc["mood"].as<String>()) + "\"}");
}

void HttpServerController::handlePose()
{
    if (!requireAuth()) return;
    JsonDocument doc;
    if (deserializeJson(doc, readBody())) return sendError(400, "INVALID_ARGUMENT", "Invalid JSON");
    MotionPose pose;
    if (!parsePoseValue(doc["pose"] | "", pose)) return sendError(400, "INVALID_ARGUMENT", "Unsupported pose");
    body_.setPose(pose);
    sendOk(String("{\"pose\":\"") + lower(doc["pose"].as<String>()) + "\"}");
}

void HttpServerController::handleMove()
{
    if (!requireAuth()) return;
    JsonDocument doc;
    if (deserializeJson(doc, readBody())) return sendError(400, "INVALID_ARGUMENT", "Invalid JSON");
    if (!doc["x"].is<int>() || !doc["y"].is<int>()) return sendError(400, "INVALID_ARGUMENT", "x and y must be integers");
    body_.moveTo(doc["x"].as<int>(), doc["y"].as<int>());
    const BodyState& state = body_.getState();
    sendOk(String("{\"x\":") + state.servoX() + ",\"y\":" + state.servoY() + "}");
}

void HttpServerController::handleReset()
{
    if (!requireAuth()) return;
    body_.setExpression(Expression::Neutral);
    body_.setMood(Mood::Calm);
    body_.setPose(MotionPose::Neutral);
    body_.stopAudio();
    sendOk("{\"reset\":true}");
}

void HttpServerController::handlePlayWav()
{
    if (!requireAuth()) return;
    if (wavBodyBuffer_ != nullptr && wavBodySize_ > 0) {
        String error;
        if (!body_.prepareWav(wavBodySize_, error)) {
            free(wavBodyBuffer_);
            wavBodyBuffer_ = nullptr;
            wavBodySize_ = 0;
            return sendError(error == "AUDIO_TOO_LARGE" ? 413 : 409, error.c_str(), error);
        }
        memcpy(body_.wavReceiveBuffer(), wavBodyBuffer_, wavBodySize_);
        const size_t size = wavBodySize_;
        free(wavBodyBuffer_);
        wavBodyBuffer_ = nullptr;
        wavBodySize_ = 0;
        if (!body_.queuePreparedWav(size, error)) {
            return sendError(400, error.c_str(), error);
        }
        return sendOk(String("{\"queued\":true,\"size\":") + size + "}");
    }
    if (wavBodyError_.length() > 0) {
        const String error = wavBodyError_;
        wavBodyError_ = "";
        return sendError(error == "AUDIO_TOO_LARGE" ? 413 : 400, error.c_str(), error);
    }

    const String plain = server_.arg("plain");
    const size_t size = plain.length();
    String error;
    if (!body_.prepareWav(size, error)) return sendError(error == "AUDIO_TOO_LARGE" ? 413 : 409, error.c_str(), error);
    memcpy(body_.wavReceiveBuffer(), plain.c_str(), size);
    if (!body_.queuePreparedWav(size, error)) return sendError(400, error.c_str(), error);
    sendOk(String("{\"queued\":true,\"size\":") + size + "}");
}

void HttpServerController::handlePlayWavBody()
{
    if (!auth_.authorize(server_)) {
        return;
    }

    HTTPRaw& raw = server_.raw();
    if (raw.status == RAW_START) {
        if (wavBodyBuffer_ != nullptr) {
            free(wavBodyBuffer_);
        }
        wavBodyBuffer_ = static_cast<uint8_t*>(ps_malloc(kMaxWavBytes));
        if (wavBodyBuffer_ == nullptr) {
            wavBodyBuffer_ = static_cast<uint8_t*>(malloc(kMaxWavBytes));
        }
        wavBodySize_ = 0;
        wavBodyError_ = wavBodyBuffer_ == nullptr ? "INTERNAL_ERROR" : "";
        return;
    }

    if (raw.status == RAW_WRITE) {
        if (wavBodyError_.length() > 0) {
            return;
        }
        if (wavBodyBuffer_ == nullptr) {
            wavBodyError_ = "INTERNAL_ERROR";
            return;
        }
        if (wavBodySize_ + raw.currentSize > kMaxWavBytes) {
            wavBodyError_ = "AUDIO_TOO_LARGE";
            return;
        }
        memcpy(wavBodyBuffer_ + wavBodySize_, raw.buf, raw.currentSize);
        wavBodySize_ += raw.currentSize;
        return;
    }

    if (raw.status == RAW_END) {
        if (wavBodySize_ == 0 && wavBodyError_.length() == 0) {
            wavBodyError_ = "AUDIO_TRANSFER_FAILED";
        }
        return;
    }

    if (raw.status == RAW_ABORTED) {
        if (wavBodyBuffer_ != nullptr) {
            free(wavBodyBuffer_);
            wavBodyBuffer_ = nullptr;
        }
        wavBodySize_ = 0;
        wavBodyError_ = "AUDIO_TRANSFER_FAILED";
    }
}

void HttpServerController::handleAudioStatus()
{
    if (!requireAuth()) return;
    const AudioState& audio = body_.getAudioState();
    sendOk(String("{\"state\":\"") + toString(audio.state()) + "\",\"playing\":" + (audio.isPlaying() ? "true" : "false") + ",\"volume\":" + audio.volume() + ",\"size\":" + audio.currentSize() + ",\"received\":" + audio.receivedSize() + "}");
}

void HttpServerController::handleAudioVolume()
{
    if (!requireAuth()) return;
    JsonDocument doc;
    if (deserializeJson(doc, readBody()) || !doc["volume"].is<int>()) return sendError(400, "INVALID_ARGUMENT", "volume must be an integer");
    const int volume = doc["volume"].as<int>();
    if (!body_.setAudioVolume(volume)) return sendError(400, "INVALID_ARGUMENT", "volume must be between 0 and 255");
    sendOk(String("{\"volume\":") + volume + "}");
}

void HttpServerController::handleAudioStop()
{
    if (!requireAuth()) return;
    body_.stopAudio();
    sendOk("{\"stopped\":true}");
}

void HttpServerController::handleMicStatus()
{
    if (!requireAuth()) return;
    const MicState& mic = body_.getMicState();
    JsonDocument doc;
    doc["available"] = mic.available();
    doc["recording"] = mic.recording();
    doc["sampleRate"] = mic.sampleRate();
    doc["channels"] = mic.channels();
    doc["lastRecordMs"] = mic.lastRecordMs();
    doc["lastPcmBytes"] = mic.lastPcmBytes();
    doc["lastWavBytes"] = mic.lastWavBytes();
    doc["lastError"] = mic.lastError();
    doc["speechServicesConfigured"] = wifi_.config().speechServicesUrl.length() > 0;
    String data;
    serializeJson(doc, data);
    sendOk(data);
}

void HttpServerController::handleMicRecord()
{
    if (!requireAuth()) return;
    JsonDocument doc;
    if (deserializeJson(doc, readBody())) return sendError(400, "INVALID_ARGUMENT", "Invalid JSON");
    const uint32_t durationMs = doc["durationMs"] | 3000;
    const String source = doc["source"] | "stackchan";
    const String url = wifi_.config().speechServicesUrl;
    if (url.length() == 0) return sendError(400, "SPEECH_SERVICES_NOT_CONFIGURED", "speechServicesUrl is not configured");

    String error;
    if (!body_.recordMicWav(durationMs, error)) return sendError(400, error.c_str(), error);

    int statusCode = 0;
    String response;
    const bool uploaded = postMultipartWav(url, source, body_.micWavBuffer(), body_.micWavSize(), statusCode, response, error);
    if (!uploaded) return sendError(502, error.c_str(), response.length() > 0 ? response : error);

    JsonDocument result;
    const MicState& mic = body_.getMicState();
    result["recorded"] = true;
    result["uploaded"] = true;
    result["durationMs"] = mic.lastRecordMs();
    result["pcmBytes"] = mic.lastPcmBytes();
    result["wavBytes"] = mic.lastWavBytes();
    result["httpStatus"] = statusCode;
    result["speechResponse"] = response;
    String data;
    serializeJson(result, data);
    sendOk(data);
}

void HttpServerController::handleWakeStatus()
{
    if (!requireAuth()) return;
    const WakeState& wake = body_.getWakeState();
    JsonDocument doc;
    doc["available"] = wake.available();
    doc["modelLoaded"] = wake.modelLoaded();
    doc["running"] = wake.running();
    doc["detected"] = wake.detected();
    doc["engine"] = wake.engine();
    doc["modelName"] = wake.modelName();
    doc["wakeWord"] = wake.wakeWord();
    doc["modelBytes"] = wake.modelBytes();
    doc["lastProbability"] = wake.lastProbability();
    doc["averageProbability"] = wake.averageProbability();
    doc["autoStart"] = wifi_.config().wakeAutoStart;
    JsonObject debug = doc["debug"].to<JsonObject>();
    debug["queuedBlocks"] = wake.queuedBlocks();
    debug["processedBlocks"] = wake.processedBlocks();
    debug["featureFrames"] = wake.featureFrames();
    debug["inferenceRuns"] = wake.inferenceRuns();
    debug["lastSamplesConsumed"] = wake.lastSamplesConsumed();
    JsonObject mic = debug["mic"].to<JsonObject>();
    mic["rms"] = wake.lastMicRms();
    mic["peak"] = wake.lastMicPeak();
    mic["maxRms"] = wake.maxMicRms();
    mic["maxPeak"] = wake.maxMicPeak();
    mic["recordingState"] = wake.micRecordingState();
    JsonObject features = debug["features"].to<JsonObject>();
    features["rawMin"] = wake.lastFeatureRawMin();
    features["rawMax"] = wake.lastFeatureRawMax();
    features["maxRawMax"] = wake.maxFeatureRawMax();
    features["min"] = wake.lastFeatureMin();
    features["max"] = wake.lastFeatureMax();
    features["maxSeen"] = wake.maxFeatureMax();
    JsonObject model = debug["model"].to<JsonObject>();
    model["lastRawOutput"] = wake.lastRawOutput();
    model["maxRawOutput"] = wake.maxRawOutput();
    JsonObject detectedDebug = debug["detected"].to<JsonObject>();
    detectedDebug["maxRawOutput"] = wake.detectedMaxRawOutput();
    detectedDebug["averageProbability"] = wake.detectedAverageProbability();
    detectedDebug["processedBlocks"] = wake.detectedProcessedBlocks();
    detectedDebug["inferenceRuns"] = wake.detectedInferenceRuns();
    doc["startedAtMs"] = wake.startedAtMs();
    doc["lastDetectedAtMs"] = wake.lastDetectedAtMs();
    doc["lastError"] = wake.lastError();
    JsonObject upload = doc["wakeUpload"].to<JsonObject>();
    upload["inProgress"] = wakeUploadInProgress_;
    upload["lastHandledDetectedAtMs"] = handledWakeDetectedAtMs_;
    upload["lastUploadAtMs"] = lastWakeUploadAtMs_;
    upload["lastHttpStatus"] = lastWakeUploadHttpStatus_;
    upload["lastError"] = lastWakeUploadError_;
    upload["lastSpeechResponse"] = lastWakeSpeechResponse_;
    String data;
    serializeJson(doc, data);
    sendOk(data);
}

void HttpServerController::handleWakeStart()
{
    if (!requireAuth()) return;
    String error;
    if (!body_.startWake(error)) return sendError(409, error.c_str(), error);
    if (!wifi_.saveWakeAutoStart(true)) return sendError(500, "DEVICE_CONFIG_SAVE_FAILED", wifi_.state().lastError);
    handleWakeStatus();
}

void HttpServerController::handleWakeStop()
{
    if (!requireAuth()) return;
    body_.stopWake();
    if (!wifi_.saveWakeAutoStart(false)) return sendError(500, "DEVICE_CONFIG_SAVE_FAILED", wifi_.state().lastError);
    handleWakeStatus();
}

void HttpServerController::handleEvents()
{
    if (!requireAuth()) return;
    JsonDocument doc;
    const size_t count = body_.input().eventCount();
    doc["count"] = count;
    JsonArray events = doc["events"].to<JsonArray>();
    for (size_t i = 0; i < count; ++i) {
        InputEvent event;
        if (body_.input().getEvent(i, event)) addEvent(events, event);
    }
    String data;
    serializeJson(doc, data);
    sendOk(data);
}

void HttpServerController::handleLatestEvent()
{
    if (!requireAuth()) return;
    JsonDocument doc;
    InputEvent event;
    if (body_.input().getLatestEvent(event)) {
        JsonObject item = doc["event"].to<JsonObject>();
        item["id"] = event.id;
        item["type"] = event.type;
        item["target"] = event.target;
        item["value"] = event.value;
        item["timestamp"] = event.timestamp;
        if (event.x >= 0 && event.y >= 0) {
            item["x"] = event.x;
            item["y"] = event.y;
        }
    } else {
        doc["event"] = nullptr;
    }
    String data;
    serializeJson(doc, data);
    sendOk(data);
}

void HttpServerController::handleClearEvents()
{
    if (!requireAuth()) return;
    body_.input().clearEvents();
    sendOk("{\"cleared\":true}");
}

void HttpServerController::handleWifiStatus()
{
    if (!requireAuth()) return;
    const NetworkState& net = wifi_.state();
    JsonDocument doc;
    doc["connected"] = net.connected;
    doc["ssid"] = net.ssid;
    doc["ip"] = net.ip;
    doc["hostname"] = net.hostname;
    doc["rssi"] = net.rssi;
    doc["auth"] = net.authEnabled;
    String data;
    serializeJson(doc, data);
    sendOk(data);
}

void HttpServerController::handleWifiConnect()
{
    if (!requireAuth()) return;
    sendOk("{\"accepted\":true}");
}

void HttpServerController::handleWifiClear()
{
    if (!requireAuth()) return;
    sendError(400, "INVALID_ARGUMENT", "Use Serial WIFI:CLEAR in Phase 8");
}

void HttpServerController::handleCameraSnapshot()
{
    sendError(501, "NOT_IMPLEMENTED", "Camera snapshot is not implemented in Phase 8");
}

}  // namespace stackchan::network
