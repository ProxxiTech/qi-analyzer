#ifndef QI_ANALYZER_H
#define QI_ANALYZER_H

#include <deque>

#include <Analyzer.h>
#include "QiAnalyzerResults.h"
#include "QiSimulationDataGenerator.h"

class QiAnalyzerSettings;

class ANALYZER_EXPORT QiAnalyzer : public Analyzer2 {
  public:
    QiAnalyzer();
    virtual ~QiAnalyzer();

    virtual void SetupResults();
    virtual void WorkerThread();

    virtual U32 GenerateSimulationData(U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channels);
    virtual U32 GetMinimumSampleRateHz();

    virtual const char* GetAnalyzerName() const;
    virtual bool        NeedsRerun();

  private:
    std::unique_ptr<QiAnalyzerSettings> mSettings;
    std::unique_ptr<QiAnalyzerResults>  mResults;
    AnalyzerChannelData*                mQi;

    QiSimulationDataGenerator mSimulationDataGenerator;
    bool                      mSimulationInitilized;

    U32 mSampleRateHz;

    U32                             mT;
    U32                             mTError;
    std::deque<std::pair<U64, U64>> mBitsForNextByte;    // value, location
    std::vector<U64>                mPreambleEdges;
    U32                             mPacketByteCount;
    bool                            mSynchronized;

  private:
    void Invalidate();
	void AdvanceToNextEdge(U64 edge_location, U64* p_next_edge_location, U64* p_next_edge_distance);
    void ProcessQiData();
    void SynchronizeQiData();
    void SaveBit(U64 location, U32 value);
};

extern "C" ANALYZER_EXPORT const char* __cdecl GetAnalyzerName();
extern "C" ANALYZER_EXPORT Analyzer* __cdecl CreateAnalyzer();
extern "C" ANALYZER_EXPORT void __cdecl DestroyAnalyzer(Analyzer* analyzer);

#endif    // QI_ANALYZER_H
