#include "DynamicSystem.h"
#include "../constants.h"

DynamicSystem::DynamicSystem() {
    this->enabled = false;
    this->samplerate = DEFAULT_SAMPLERATE;
    this->bass.SetSamplingRate(this->samplerate);
    this->bass.Reset();
}

void DynamicSystem::Process(float *samples, uint32_t size) {
    if (this->enabled) {
        this->bass.FilterSamples(samples, size);
    }
}

void DynamicSystem::Reset() {
    this->bass.SetSamplingRate(this->samplerate);
    this->bass.Reset();
}

void DynamicSystem::SetBassGain(float gain) {
    this->bass.SetBassGain(gain);
}

void DynamicSystem::SetEnable(bool enable) {
    if (this->enabled != enable) {
        this->enabled = enable;
        if (enable) {
            Reset();
        }
    }
}

void DynamicSystem::SetSideGain(float gainX, float gainY) {
    this->bass.SetSideGain(gainX, gainY);
}

void DynamicSystem::SetXCoeffs(uint32_t low, uint32_t high) {
    this->bass.SetFilterXPassFrequency(low, high);
}

void DynamicSystem::SetYCoeffs(uint32_t low, uint32_t high) {
    this->bass.SetFilterYPassFrequency(low, high);
}

void DynamicSystem::SetSamplingRate(uint32_t samplerate) {
    if (this->samplerate != samplerate) {
        this->samplerate = samplerate;
        this->bass.SetSamplingRate(samplerate);
    }
}

DynamicSystem::~DynamicSystem() {

}