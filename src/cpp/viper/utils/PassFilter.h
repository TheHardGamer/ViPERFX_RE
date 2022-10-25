#pragma once

#include <cstdint>
#include "IIR_NOrder_BW_LH.h"

class PassFilter {
public:
    PassFilter();
    ~PassFilter();

    void Reset();
    void ProcessFrames(float *buffer, uint32_t size);
    void SetSamplingRate(uint32_t samplingRate);

private:
    IIR_NOrder_BW_LH *filters[4];
    uint32_t samplingRate;
};
