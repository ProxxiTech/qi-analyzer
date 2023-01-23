#include "QiSimulationDataGenerator.h"
#include "QiAnalyzerSettings.h"

#include <AnalyzerHelpers.h>

static const U32 kBitRate = 2000;

QiSimulationDataGenerator::QiSimulationDataGenerator() : mSettings(nullptr), mSimulationSampleRateHz(0), mT(0), mSimValue(0) {}

QiSimulationDataGenerator::~QiSimulationDataGenerator() {}

void QiSimulationDataGenerator::Initialize(U32 simulation_sample_rate, QiAnalyzerSettings* settings) {
    mSimulationSampleRateHz = simulation_sample_rate;
    mSettings               = settings;

    mQiSimulationData.SetChannel(mSettings->mInputChannel);
    mQiSimulationData.SetSampleRate(simulation_sample_rate);
    mQiSimulationData.SetInitialBitState(BIT_LOW);

    double half_period = 1.0 / double(kBitRate * 2);
    half_period *= 1000000.0;
    mT        = UsToSamples(half_period);
    mSimValue = 1;

    mQiSimulationData.Advance(U32(mT * 8));
}

U32 QiSimulationDataGenerator::GenerateSimulationData(U64                           largest_sample_requested,
                                                      U32                           sample_rate,
                                                      SimulationChannelDescriptor** simulation_channel) {
    U64 adjusted_largest_sample_requested =
        AnalyzerHelpers::AdjustSimulationTargetSample(largest_sample_requested, sample_rate, mSimulationSampleRateHz);

    while (mQiSimulationData.GetCurrentSampleNumber() < adjusted_largest_sample_requested) {
        // preamble of 11-25 1-bits for synchronization
        for (U32 i = 0; i < 25; ++i)
            SimWriteBit(1);
        SimWriteByte(mSimValue++);
        SimWriteByte(mSimValue++);
        SimWriteByte(mSimValue++);
        mQiSimulationData.Advance(U32(mT * 8));
    }

    *simulation_channel = &mQiSimulationData;
    return 1;
}

U64 QiSimulationDataGenerator::UsToSamples(U64 us) {
    return (mSimulationSampleRateHz * us) / 1000000;
}

U64 QiSimulationDataGenerator::UsToSamples(double us) {
    return U64((mSimulationSampleRateHz * us) / 1000000.0);
}

U64 QiSimulationDataGenerator::SamplesToUs(U64 samples) {
    return (samples * 1000000) / mSimulationSampleRateHz;
}

void QiSimulationDataGenerator::SimWriteByte(U64 value) {
    U32 bits_per_xfer = 11;

    for (U32 i = 0; i < bits_per_xfer; ++i) {
        SimWriteBit((value >> i) & 0x1);
    }
}

void QiSimulationDataGenerator::SimWriteBit(U32 bit) {
    BitState start_bit_state = mQiSimulationData.GetCurrentBitState();
    mQiSimulationData.Transition();
    mQiSimulationData.Advance(U32(mT));
    if (bit == 1) {
        mQiSimulationData.Transition();
    }
    mQiSimulationData.Advance(U32(mT));
}
