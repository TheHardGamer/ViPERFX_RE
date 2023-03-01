#include <cstring>
#include <cmath>
#include "SoftwareLimiter.h"

SoftwareLimiter::SoftwareLimiter() {
    this->ready = false;
    this->unknown4 = 0;
    this->unknown2 = 1.0;
    this->gate = 0.999999;
    this->unknown3 = 1.0;
    this->unknown1 = 1.0;

    Reset();
}

float SoftwareLimiter::Process(float sample) {
    bool bVar1;
    float *pfVar2;
    uint uVar3;
    uint uVar4;
    int iVar5;
    uint uVar6;
    uint *puVar7;
    uint uVar8;
    float fVar9;
    float fVar10;
    float gate;
    float abs_sample;

    gate = this->gate;
    abs_sample = std::abs(sample);
    if (abs_sample < gate) {
        if (this->ready) goto LAB_0006d86c;
        uVar8 = this->unknown4;
    }
    else {
        if (!this->ready) {
            iVar5 = 0x200;
            pfVar2 = this->arr512;
            do {
                iVar5 = iVar5 + -1;
                *pfVar2 = 0.0;
                pfVar2 = pfVar2 + 1;
            } while (iVar5 != 0);
        }
        this->ready = true;
LAB_0006d86c:
        uVar3 = 8;
        puVar7 = &this->unknown4;
        uVar8 = *puVar7;
        uVar4 = uVar8;
        do {
            iVar5 = 2 << (uVar3 & 0xff);
            uVar6 = uVar4 ^ 1;
            puVar7[uVar4 - iVar5] = (uint)abs_sample;
            uVar4 = (int)uVar4 >> 1;
            if (abs_sample < (float)puVar7[uVar6 - iVar5]) {
                abs_sample = (float)puVar7[uVar6 - iVar5];
            }
            uVar3 = uVar3 - 1;
        } while (uVar3 != 0);
        gate = this->gate;
        if (gate < abs_sample) {
            bVar1 = this->ready;
            fVar10 = this->unknown1;
            uVar4 = uVar8 + 1 & 0xff;
            this->arr256[uVar8] = sample;
            this->unknown4 = uVar4;
            if (bVar1) {
                fVar10 = gate / abs_sample;
            }
            abs_sample = this->arr256[uVar4];
            goto LAB_0006d8fc;
        }
        this->ready = false;
    }
    fVar10 = this->unknown1;
    this->arr256[uVar8] = sample;
    uVar8 = uVar8 + 1 & 0xff;
    this->unknown4 = uVar8;
    abs_sample = this->arr256[uVar8];
LAB_0006d8fc:
    fVar9 = this->unknown2 * 0.9999 + 0.0001;
    fVar10 = fVar10 * 0.0999 + this->unknown3 * 0.8999;
    bVar1 = fVar10 < fVar9;
    this->unknown3 = fVar10;
    if (bVar1) {
        fVar9 = fVar10;
    }
    if (bVar1) {
        this->unknown2 = fVar10;
    }
    if (!bVar1) {
        this->unknown2 = fVar9;
    }
    fVar9 = abs_sample * fVar9;
    fVar10 = std::abs(fVar9);
    if (gate <= fVar10) {
        fVar9 = gate / std::abs(abs_sample);
    }
    if (gate <= fVar10) {
        this->unknown2 = fVar9;
        fVar9 = abs_sample * fVar9;
    }
    return fVar9;
}

void SoftwareLimiter::Reset() {
    memset(this->arr256, 0, sizeof(this->arr256));
    memset(this->arr512, 0, sizeof(this->arr512));
    this->ready = false;
    this->unknown4 = 0;
    this->unknown2 = 1.0;
    this->unknown3 = 1.0;
}

void SoftwareLimiter::SetGate(float gate) {
    this->gate = gate;
}