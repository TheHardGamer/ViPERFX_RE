#include <cerrno>
#include <cstring>
#include <cmath>
#include <chrono>
#include "ViperContext.h"
#include "log.h"

ViperContext::ViperContext() :
        config({}),
        isConfigValid(false),
        buffer(std::vector<float>()),
        bufferFrameCount(0),
        enabled(false) {
    VIPER_LOGI("ViperContext created");
}

int ViperContext::handleSetConfig(effect_config_t *newConfig) {
    VIPER_LOGI("Checking input and output configuration ...");

    VIPER_LOGI("Input buffer frame count: %ld", newConfig->inputCfg.buffer.frameCount);
    VIPER_LOGI("Input sampling rate: %d", newConfig->inputCfg.samplingRate);
    VIPER_LOGI("Input channels: %d", newConfig->inputCfg.channels);
    VIPER_LOGI("Input format: %d", newConfig->inputCfg.format);
    VIPER_LOGI("Input access mode: %d", newConfig->inputCfg.accessMode);
    VIPER_LOGI("Output buffer frame count: %ld", newConfig->outputCfg.buffer.frameCount);
    VIPER_LOGI("Output sampling rate: %d", newConfig->outputCfg.samplingRate);
    VIPER_LOGI("Output channels: %d", newConfig->outputCfg.channels);
    VIPER_LOGI("Output format: %d", newConfig->outputCfg.format);
    VIPER_LOGI("Output access mode: %d", newConfig->outputCfg.accessMode);

    isConfigValid = false;

    if (newConfig->inputCfg.buffer.frameCount != newConfig->outputCfg.buffer.frameCount) {
        VIPER_LOGE("ViPER4Android disabled, reason [in.FC = %ld, out.FC = %ld]",
                   newConfig->inputCfg.buffer.frameCount, newConfig->outputCfg.buffer.frameCount);
//        disableReason = "Invalid frame count";
        return 0;
    }

    if (newConfig->inputCfg.samplingRate != newConfig->outputCfg.samplingRate) {
        VIPER_LOGE("ViPER4Android disabled, reason [in.SR = %d, out.SR = %d]",
                   newConfig->inputCfg.samplingRate, newConfig->outputCfg.samplingRate);
//        disableReason = "Invalid sampling rate";
        return 0;
    }

    if (newConfig->inputCfg.samplingRate > 48000) {
        VIPER_LOGE("ViPER4Android disabled, reason [SR out of range]");
//        disableReason = "Sampling rate out of range";
        return 0;
    }

    if (newConfig->inputCfg.channels != newConfig->outputCfg.channels) {
        VIPER_LOGE("ViPER4Android disabled, reason [in.CH = %d, out.CH = %d]",
                   newConfig->inputCfg.channels, newConfig->outputCfg.channels);
//        disableReason = "Invalid channel count";
        return 0;
    }

    if (newConfig->inputCfg.channels != AUDIO_CHANNEL_OUT_STEREO) {
        VIPER_LOGE("ViPER4Android disabled, reason [CH != 2]");
//        disableReason = "Channel count != 2";
        return 0;
    }

    if (newConfig->inputCfg.format != AUDIO_FORMAT_PCM_16_BIT &&
        newConfig->inputCfg.format != AUDIO_FORMAT_PCM_32_BIT &&
        newConfig->inputCfg.format != AUDIO_FORMAT_PCM_FLOAT) {
        VIPER_LOGE("ViPER4Android disabled, reason [in.FMT = %d]", newConfig->inputCfg.format);
        VIPER_LOGE("We only accept AUDIO_FORMAT_PCM_16_BIT, AUDIO_FORMAT_PCM_32_BIT and AUDIO_FORMAT_PCM_FLOAT input format!");
//        disableReason = "Invalid input format";
        return 0;
    }

    if (newConfig->outputCfg.format != AUDIO_FORMAT_PCM_16_BIT &&
        newConfig->outputCfg.format != AUDIO_FORMAT_PCM_32_BIT &&
        newConfig->outputCfg.format != AUDIO_FORMAT_PCM_FLOAT) {
        VIPER_LOGE("ViPER4Android disabled, reason [out.FMT = %d]", newConfig->outputCfg.format);
        VIPER_LOGE("We only accept AUDIO_FORMAT_PCM_16_BIT, AUDIO_FORMAT_PCM_32_BIT and AUDIO_FORMAT_PCM_FLOAT output format!");
//        disableReason = "Invalid output format";
        return 0;
    }

    VIPER_LOGI("Input and output configuration checked.");

    // Config
    config = *newConfig;
    isConfigValid = true;
//    disableReason = "";
    // Processing buffer
    buffer.resize(newConfig->inputCfg.buffer.frameCount * 2);
    bufferFrameCount = newConfig->inputCfg.buffer.frameCount;
    // ViPER
    viper.samplingRate = newConfig->inputCfg.samplingRate;
    viper.ResetAllEffects();

    return 0;
}

int32_t ViperContext::handleSetParam(effect_param_t *pCmdParam, void *pReplyData) {
    // The value offset of an effect parameter is computed by rounding up
    // the parameter size to the next 32 bit alignment.
    uint32_t vOffset = ((pCmdParam->psize + sizeof(int32_t) - 1) / sizeof(int32_t)) * sizeof(int32_t);

    *(int *) pReplyData = 0;

    int param = *(int *) (pCmdParam->data);
    int *intValues = (int *) (pCmdParam->data + vOffset);
    switch (pCmdParam->vsize) {
        case sizeof(int): {
            viper.DispatchCommand(param, intValues[0], 0, 0, 0, 0, nullptr);
            return 0;
        }
        case sizeof(int) * 2: {
            viper.DispatchCommand(param, intValues[0], intValues[1], 0, 0, 0, nullptr);
            return 0;
        }
        case sizeof(int) * 3: {
            viper.DispatchCommand(param, intValues[0], intValues[1], intValues[2], 0, 0, nullptr);
            return 0;
        }
        case sizeof(int) * 4: {
            viper.DispatchCommand(param, intValues[0], intValues[1], intValues[2], intValues[3], 0, nullptr);
            return 0;
        }
        case 256:
        case 1024: {
            uint32_t arrSize = *(uint32_t *) (pCmdParam->data + vOffset);
            signed char *arr = (signed char *) (pCmdParam->data + vOffset + sizeof(uint32_t));
            viper.DispatchCommand(param, 0, 0, 0, 0, arrSize, arr);
            return 0;
        }
        case 8192: {
            int value1 = *(int *) (pCmdParam->data + vOffset);
            uint32_t arrSize = *(uint32_t *) (pCmdParam->data + vOffset + sizeof(int));
            signed char *arr = (signed char *) (pCmdParam->data + vOffset + sizeof(int) + sizeof(uint32_t));
            viper.DispatchCommand(param, value1, 0, 0, 0, arrSize, arr);
            return 0;
        }
        default: {
            return -EINVAL;
        }
    }
}

int32_t ViperContext::handleGetParam(effect_param_t *pCmdParam, effect_param_t *pReplyParam, uint32_t *pReplySize) {
    // The value offset of an effect parameter is computed by rounding up
    // the parameter size to the next 32 bit alignment.
    uint32_t vOffset = ((pCmdParam->psize + sizeof(int32_t) - 1) / sizeof(int32_t)) * sizeof(int32_t);

    VIPER_LOGD("viperInterfaceCommand() EFFECT_CMD_GET_PARAM called with data = %d, psize = %d, vsize = %d", *(uint32_t *) pCmdParam->data, pCmdParam->psize, pCmdParam->vsize);

    memcpy(pReplyParam, pCmdParam, sizeof(effect_param_t) + pCmdParam->psize);

    switch (*(uint32_t *) pCmdParam->data) {
        case PARAM_GET_ENABLED: {
            pReplyParam->status = 0;
            pReplyParam->vsize = sizeof(int32_t);
            *(int32_t *) (pReplyParam->data + vOffset) = enabled;
            *pReplySize = sizeof(effect_param_t) + pReplyParam->psize + vOffset + pReplyParam->vsize;
            return 0;
        }
        case PARAM_GET_CONFIGURE: {
            pReplyParam->status = 0;
            pReplyParam->vsize = sizeof(int32_t);
            *(int32_t *) (pReplyParam->data + vOffset) = isConfigValid;
            *pReplySize = sizeof(effect_param_t) + pReplyParam->psize + vOffset + pReplyParam->vsize;
            return 0;
        }
        case PARAM_GET_STREAMING: { // Is processing
            auto now = std::chrono::system_clock::now();
            auto now_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(now);

            uint64_t currentMs = now_ms.time_since_epoch().count();
            uint64_t lastProcessTime = viper.processTimeMs;

            uint64_t diff;
            if (currentMs > lastProcessTime) {
                diff = currentMs - lastProcessTime;
            } else {
                diff = lastProcessTime - currentMs;
            }

            pReplyParam->status = 0;
            pReplyParam->vsize = sizeof(int32_t);
            *(int32_t *) (pReplyParam->data + vOffset) = diff > 5000 ? 0 : 1;
            *pReplySize = sizeof(effect_param_t) + pReplyParam->psize + vOffset + pReplyParam->vsize;
            return 0;
        }
        case PARAM_GET_SAMPLING_RATE: {
            pReplyParam->status = 0;
            pReplyParam->vsize = sizeof(uint32_t);
            *(uint32_t *) (pReplyParam->data + vOffset) = viper.samplingRate;
            *pReplySize = sizeof(effect_param_t) + pReplyParam->psize + vOffset + pReplyParam->vsize;
            return 0;
        }
        case PARAM_GET_CONVOLUTION_KERNEL_ID: {
            pReplyParam->status = 0;
            pReplyParam->vsize = sizeof(uint32_t);
            *(uint32_t *) (pReplyParam->data + vOffset) = viper.convolver->GetKernelID();
            *pReplySize = sizeof(effect_param_t) + pReplyParam->psize + vOffset + pReplyParam->vsize;
            return 0;
        }
        case PARAM_GET_DRIVER_VERSION_CODE: {
            pReplyParam->status = 0;
            pReplyParam->vsize = sizeof(uint32_t);
            *(int32_t *) (pReplyParam->data + vOffset) = VERSION_CODE;
            *pReplySize = sizeof(effect_param_t) + pReplyParam->psize + vOffset + pReplyParam->vsize;
            return 0;
        }
        case PARAM_GET_DRIVER_VERSION_NAME: {
            pReplyParam->status = 0;
            pReplyParam->vsize = strlen(VERSION_NAME);
            memcpy(pReplyParam->data + vOffset, VERSION_NAME, pReplyParam->vsize);
            *pReplySize = sizeof(effect_param_t) + pReplyParam->psize + vOffset + pReplyParam->vsize;
            return 0;
        }
        default: {
            return -EINVAL;
        }
    }
}

#define SET_INT32(ptr, value) (*(int32_t *) (ptr) = (value))
#define GET_REPLY_SIZE(ptr) (ptr == nullptr ? 0 : *ptr)

int32_t ViperContext::handleCommand(uint32_t cmdCode, uint32_t cmdSize, void *pCmdData, uint32_t *replySize, void *pReplyData) {
    switch (cmdCode) {
        case EFFECT_CMD_INIT: {
            if (GET_REPLY_SIZE(replySize) != sizeof(int32_t) || pReplyData == nullptr) {
                VIPER_LOGE("EFFECT_CMD_INIT called with invalid replySize = %d, pReplyData = %p, expected replySize = %lu", GET_REPLY_SIZE(replySize), pReplyData, sizeof(int32_t));
                return -EINVAL;
            }
            SET_INT32(pReplyData, 0);
            return 0;
        }
        case EFFECT_CMD_SET_CONFIG: {
            if (cmdSize < sizeof(effect_config_t) || pCmdData == nullptr || GET_REPLY_SIZE(replySize) != sizeof(int32_t) || pReplyData == nullptr) {
                VIPER_LOGE("EFFECT_CMD_SET_CONFIG called with invalid cmdSize = %d, pCmdData = %p, replySize = %d, pReplyData = %p, expected cmdSize = %lu, replySize = %lu", cmdSize, pCmdData, GET_REPLY_SIZE(replySize), pReplyData, sizeof(effect_config_t), sizeof(int32_t));
                return -EINVAL;
            }
            SET_INT32(pReplyData, handleSetConfig((effect_config_t *) pCmdData));
            return 0;
        }
        case EFFECT_CMD_RESET: {
            if (GET_REPLY_SIZE(replySize) != sizeof(int32_t) || pReplyData == nullptr) {
                VIPER_LOGE("EFFECT_CMD_RESET called with invalid replySize = %d, pReplyData = %p, expected replySize = %lu", GET_REPLY_SIZE(replySize), pReplyData, sizeof(int32_t));
                return -EINVAL;
            }
            viper.ResetAllEffects();
            SET_INT32(pReplyData, 0);
            return 0;
        }
        case EFFECT_CMD_ENABLE: {
            if (GET_REPLY_SIZE(replySize) != sizeof(int32_t) || pReplyData == nullptr) {
                VIPER_LOGE("EFFECT_CMD_ENABLE called with invalid replySize = %d, pReplyData = %p, expected replySize = %lu", GET_REPLY_SIZE(replySize), pReplyData, sizeof(int32_t));
                return -EINVAL;
            }
            viper.ResetAllEffects();
            enabled = true;
            SET_INT32(pReplyData, 0);
            return 0;
        }
        case EFFECT_CMD_DISABLE: {
            if (GET_REPLY_SIZE(replySize) != sizeof(int32_t) || pReplyData == nullptr) {
                VIPER_LOGE("EFFECT_CMD_DISABLE called with invalid replySize = %d, pReplyData = %p, expected replySize = %lu", GET_REPLY_SIZE(replySize), pReplyData, sizeof(int32_t));
                return -EINVAL;
            }
            enabled = false;
            SET_INT32(pReplyData, 0);
            return 0;
        }
        case EFFECT_CMD_SET_PARAM: {
            if (cmdSize < sizeof(effect_param_t) || pCmdData == nullptr || GET_REPLY_SIZE(replySize) != sizeof(int32_t) || pReplyData == nullptr) {
                VIPER_LOGE("EFFECT_CMD_SET_PARAM called with invalid cmdSize = %d, pCmdData = %p, replySize = %d, pReplyData = %p, expected cmdSize = %lu, replySize = %lu", cmdSize, pCmdData, GET_REPLY_SIZE(replySize), pReplyData, sizeof(effect_param_t), sizeof(int32_t));
                return -EINVAL;
            }
            return handleSetParam((effect_param_t *) pCmdData, pReplyData);
        }
        case EFFECT_CMD_GET_PARAM: {
            if (cmdSize < sizeof(effect_param_t) || pCmdData == nullptr || GET_REPLY_SIZE(replySize) < sizeof(effect_param_t) || pReplyData == nullptr) {
                VIPER_LOGE("EFFECT_CMD_GET_PARAM called with invalid cmdSize = %d, pCmdData = %p, replySize = %d, pReplyData = %p, expected cmdSize = %lu, replySize = %lu", cmdSize, pCmdData, GET_REPLY_SIZE(replySize), pReplyData, sizeof(effect_param_t), sizeof(effect_param_t));
                return -EINVAL;
            }
            return handleGetParam((effect_param_t *) pCmdData, (effect_param_t *) pReplyData, replySize);
        }
        case EFFECT_CMD_GET_CONFIG: {
            if (GET_REPLY_SIZE(replySize) != sizeof(effect_config_t) || pReplyData == nullptr) {
                VIPER_LOGE("EFFECT_CMD_GET_CONFIG called with invalid replySize = %d, pReplyData = %p, expected replySize = %lu", GET_REPLY_SIZE(replySize), pReplyData, sizeof(effect_config_t));
                return -EINVAL;
            }
            *(effect_config_t *) pReplyData = config;
            return 0;
        }
        default: {
            VIPER_LOGE("viperInterfaceCommand called with unknown command: %d", cmdCode);
            return -EINVAL;
        }
    }
}

static void pcm16ToFloat(float *dst, const int16_t *src, size_t frameCount) {
    for (size_t i = 0; i < frameCount * 2; i++) {
        dst[i] = static_cast<float>(src[i]) / static_cast<float>(1 << 15);
    }
}

static void pcm32ToFloat(float *dst, const int32_t *src, size_t frameCount) {
    for (size_t i = 0; i < frameCount * 2; i++) {
        dst[i] = static_cast<float>(src[i]) / static_cast<float>(1 << 31);
    }
}

static void floatToFloat(float *dst, const float *src, size_t frameCount, bool accumulate) {
    if (accumulate) {
        for (size_t i = 0; i < frameCount * 2; i++) {
            dst[i] += src[i];
        }
    } else {
        memcpy(dst, src, frameCount * 2 * sizeof(float));
    }
}

static void floatToPcm16(int16_t *dst, const float *src, size_t frameCount, bool accumulate) {
    if (accumulate) {
        for (size_t i = 0; i < frameCount * 2; i++) {
            dst[i] += static_cast<int16_t>(std::round(src[i] * static_cast<float>(1 << 15)));
        }
    } else {
        for (size_t i = 0; i < frameCount * 2; i++) {
            dst[i] = static_cast<int16_t>(std::round(src[i] * static_cast<float>(1 << 15)));
        }
    }
}

static void floatToPcm32(int32_t *dst, const float *src, size_t frameCount, bool accumulate) {
    if (accumulate) {
        for (size_t i = 0; i < frameCount * 2; i++) {
            dst[i] += static_cast<int32_t>(std::round(src[i] * static_cast<float>(1 << 31)));
        }
    } else {
        for (size_t i = 0; i < frameCount * 2; i++) {
            dst[i] = static_cast<int32_t>(std::round(src[i] * static_cast<float>(1 << 31)));
        }
    }
}

static audio_buffer_t *getBuffer(buffer_config_s *config, audio_buffer_t *buffer) {
    if (buffer != nullptr) return buffer;
    return &config->buffer;
}

int32_t ViperContext::process(audio_buffer_t *inBuffer, audio_buffer_t *outBuffer) {
    if (!isConfigValid) {
        return -EINVAL;
    }

    if (!enabled) {
        return -ENODATA;
    }
    
    inBuffer = getBuffer(&config.inputCfg, inBuffer);
    outBuffer = getBuffer(&config.outputCfg, outBuffer);
    if (inBuffer == nullptr || outBuffer == nullptr ||
        inBuffer->raw == nullptr || outBuffer->raw == nullptr ||
        inBuffer->frameCount != outBuffer->frameCount ||
        inBuffer->frameCount == 0) {
        return -EINVAL;
    }

    size_t frameCount = inBuffer->frameCount;
    if (frameCount > bufferFrameCount) {
        buffer.resize(frameCount * 2);
        bufferFrameCount = frameCount;
    }

    switch (config.inputCfg.format) {
        case AUDIO_FORMAT_PCM_16_BIT:
            pcm16ToFloat(buffer.data(), inBuffer->s16, frameCount);
            break;
        case AUDIO_FORMAT_PCM_32_BIT:
            pcm32ToFloat(buffer.data(), inBuffer->s32, frameCount);
            break;
        case AUDIO_FORMAT_PCM_FLOAT:
            floatToFloat(buffer.data(), inBuffer->f32, frameCount, false);
            break;
        default:
            return -EINVAL;
    }

    viper.processBuffer(buffer.data(), frameCount);

    const bool accumulate = config.outputCfg.accessMode == EFFECT_BUFFER_ACCESS_ACCUMULATE;
    switch (config.outputCfg.format) {
        case AUDIO_FORMAT_PCM_16_BIT:
            floatToPcm16(outBuffer->s16, buffer.data(), frameCount, accumulate);
            break;
        case AUDIO_FORMAT_PCM_32_BIT:
            floatToPcm32(outBuffer->s32, buffer.data(), frameCount, accumulate);
            break;
        case AUDIO_FORMAT_PCM_FLOAT:
            floatToFloat(outBuffer->f32, buffer.data(), frameCount, accumulate);
            break;
        default:
            return -EINVAL;
    }

    return 0;
}