#ifndef QI_SIMULATION_DATA_GENERATOR
#define QI_SIMULATION_DATA_GENERATOR

#include <SimulationChannelDescriptor.h>
#include <string>
class QiAnalyzerSettings;

class QiSimulationDataGenerator {
  public:
    QiSimulationDataGenerator();
    ~QiSimulationDataGenerator();

    void Initialize(U32 simulation_sample_rate, QiAnalyzerSettings* settings);
    U32  GenerateSimulationData(U64                           newest_sample_requested,
                                U32                           sample_rate,
                                SimulationChannelDescriptor** simulation_channel);

  protected:
    QiAnalyzerSettings* mSettings;
    U32                 mSimulationSampleRateHz;

    SimulationChannelDescriptor mQiSimulationData;

  protected:
    U64 UsToSamples(U64 us);
    U64 UsToSamples(double us);
    U64 SamplesToUs(U64 samples);

    void SimWriteByte(U64 value);
    void SimWriteBit(U32 bit);

    U64 mT;
    U64 mSimValue;
};
#endif    // QI_SIMULATION_DATA_GENERATOR