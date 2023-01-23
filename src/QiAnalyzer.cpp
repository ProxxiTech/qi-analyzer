#include <sstream>
#include <ios>
#include <algorithm>

#include "QiAnalyzer.h"
#include "QiAnalyzerSettings.h"
#include <AnalyzerChannelData.h>

static const U32 kBitRate = 2000;

QiAnalyzer::QiAnalyzer()
    : Analyzer2()
    , mSettings(new QiAnalyzerSettings())
    , mQi(nullptr)
    , mSimulationInitilized(false)
    , mSampleRateHz(0)
    , mT(0)
    , mTError(0)
    , mPacketByteCount(0)
    , mSynchronized(false) {
    SetAnalyzerSettings(mSettings.get());
    UseFrameV2();

    mPreambleEdges.reserve(64);
}

QiAnalyzer::~QiAnalyzer() {
    KillThread();
}

void QiAnalyzer::SetupResults() {
    mResults.reset(new QiAnalyzerResults(this, mSettings.get()));
    SetAnalyzerResults(mResults.get());
    mResults->AddChannelBubblesWillAppearOn(mSettings->mInputChannel);
}

void QiAnalyzer::WorkerThread() {
    mSampleRateHz = GetSampleRate();

    mQi = GetAnalyzerChannelData(mSettings->mInputChannel);

    double half_peroid = 1.0 / double(kBitRate * 2);
    half_peroid *= 1000000.0;
    mT      = U32((mSampleRateHz * half_peroid) / 1000000.0);
    mTError = mT / 2;
    if (mTError < 3)
        mTError = 3;

    Invalidate();
    mQi->AdvanceToNextEdge();

    for (;;) {
        SynchronizeQiData();
        ProcessQiData();

        CheckIfThreadShouldExit();
    }
}

void QiAnalyzer::Invalidate() {
    mSynchronized    = false;
    mPacketByteCount = 0;
    mPreambleEdges.clear();
    mBitsForNextByte.clear();
}

void QiAnalyzer::AdvanceToNextEdge(U64 edge_location, U64* p_next_edge_location, U64* p_next_edge_distance) {
    mQi->AdvanceToNextEdge();
    U64 next_edge_location = mQi->GetSampleNumber();

    U64 next_edge_distance = next_edge_location - edge_location;

    while (next_edge_distance <= mTError) {
        mQi->AdvanceToNextEdge();
        next_edge_location = mQi->GetSampleNumber();

        next_edge_distance = next_edge_location - edge_location;

        // We need two glitch edges as this is a differential signal, so the signal should bounce back to our expected state
        mQi->AdvanceToNextEdge();
        next_edge_location = mQi->GetSampleNumber();

        next_edge_distance = next_edge_location - edge_location;
    }

    *p_next_edge_location = next_edge_location;
    *p_next_edge_distance = next_edge_distance;
}

void QiAnalyzer::SynchronizeQiData() {
    while (mSynchronized == false) {
        CheckIfThreadShouldExit();

        U64 edge_location = mQi->GetSampleNumber();
		BitState bit_state = mQi->GetBitState();

        U64 next_edge_location, next_edge_distance;
        AdvanceToNextEdge(edge_location, &next_edge_location, &next_edge_distance);

		if (bit_state == BIT_HIGH) {
			if ((next_edge_distance > (mT - mTError)) && (next_edge_distance < (mT + mTError))) {
				// short
				U64 next_next_edge_location, next_next_edge_distance;
				AdvanceToNextEdge(next_edge_location, &next_next_edge_location, &next_next_edge_distance);

				if ((next_next_edge_distance > (mT - mTError)) && (next_next_edge_distance < (mT + mTError))) {
					// short again -> 1-bit
					mPreambleEdges.push_back(edge_location);
					mPreambleEdges.push_back(next_edge_location);

					mResults->AddMarker(edge_location + mT, AnalyzerResults::Dot, mSettings->mInputChannel);
				} else {
					// back to idle.
					mResults->AddMarker(edge_location + mT, AnalyzerResults::ErrorDot, mSettings->mInputChannel);
					Invalidate();
				}
			} else if ((next_edge_distance > ((2 * mT) - mTError)) && (next_edge_distance < ((2 * mT) + mTError))) {
				// long -> 0-bit
				size_t count = mPreambleEdges.size() / 2;
				if ((count >= 11) && (count <= 25)) {
					mSynchronized = true;

					// The end of the preamble marks the start of a new packet
					mPacketByteCount = 0;

					// for (U32 i = 0; i < count; ++i) {
					//     U32 idx = i * 2;
					//     SaveBit(mPreambleEdges[idx], 1);
					// }

					// TODO: Use the preamble for synchronization of the clock edges, low-time/high-time, and rise-time/fall-time!

					// Record the first bit (the start bit). ProcessQuData() will continue from the edge of the first data bit.
					SaveBit(edge_location, 0);
				} else {
					// back to idle.
					mResults->AddMarker(edge_location + mT, AnalyzerResults::ErrorDot, mSettings->mInputChannel);
					Invalidate();
				}

				break;
			} else {
				// back to idle.
				mResults->AddMarker(edge_location + mT, AnalyzerResults::ErrorDot, mSettings->mInputChannel);
				Invalidate();
			}
		} else {
			// the first edge is always low->high, so we ignore any high->low transitions while synchronizing
			mResults->AddMarker(edge_location + mT, AnalyzerResults::DownArrow, mSettings->mInputChannel);
			Invalidate();
		}
    }
}

void QiAnalyzer::ProcessQiData() {
    if (mSynchronized == true) {
        // We're on the clock edge of a data, parity, or stop bit, as the start bit is recorded by SynchronizeQiData().
        U64 edge_location = mQi->GetSampleNumber();

        U64 next_edge_location, next_edge_distance;
        AdvanceToNextEdge(edge_location, &next_edge_location, &next_edge_distance);

        if ((next_edge_distance > (mT - mTError)) && (next_edge_distance < (mT + mTError))) {
            // short
            U64 next_next_edge_location, next_next_edge_distance;
            AdvanceToNextEdge(next_edge_location, &next_next_edge_location, &next_next_edge_distance);

            if ((next_next_edge_distance > (mT - mTError)) && (next_next_edge_distance < (mT + mTError))) {
                // short again -> 1-bit
                SaveBit(edge_location, 1);
            } else {
                // not synced anymore.
                mResults->AddMarker(edge_location + mT, AnalyzerResults::ErrorSquare, mSettings->mInputChannel);
                Invalidate();
                return;
            }
        } else if ((next_edge_distance > ((2 * mT) - mTError)) && (next_edge_distance < ((2 * mT) + mTError))) {
            // long -> 0-bit
            SaveBit(edge_location, 0);
        } else {
            // not synced anymore.
            mResults->AddMarker(edge_location + mT, AnalyzerResults::ErrorSquare, mSettings->mInputChannel);
            Invalidate();
            return;
        }
    }
}

void QiAnalyzer::SaveBit(U64 location, U32 value) {
    mBitsForNextByte.push_back(std::pair<U64, U64>(value, location));

    const U32 bit_count = 11;
    if (mBitsForNextByte.size() == bit_count) {
        U64 byte = 0;
        for (U32 i = 0; i < bit_count; i++) {
            std::pair<U64, U64> bit = mBitsForNextByte[i];

            U64 value = bit.first;
            byte |= value << i;
        }

        // Remove the start, parity, and stop bits so that we're left with just the data bits
        std::pair<U64, U64> start_bit = mBitsForNextByte.front();
        mBitsForNextByte.pop_front();
        std::pair<U64, U64> stop_bit = mBitsForNextByte.back();
        mBitsForNextByte.pop_back();
        std::pair<U64, U64> parity_bit = mBitsForNextByte.back();
        mBitsForNextByte.pop_back();

        U64 ones = 0;
        for (U32 i = 0; i < mBitsForNextByte.size(); i++) {
            std::pair<U64, U64> bit = mBitsForNextByte[i];

            U64 value = bit.first;
            ones += value;
        }
        U32 parity = 1 - (ones % 2);

        mResults->AddMarker(start_bit.second + mT, AnalyzerResults::Start, mSettings->mInputChannel);

        for (U32 i = 0; i < mBitsForNextByte.size(); i++) {
            std::pair<U64, U64> bit = mBitsForNextByte[i];

            U64 value    = bit.first;
            U64 location = bit.second + mT;

            AnalyzerResults::MarkerType marker = (value == 0) ? AnalyzerResults::Zero : AnalyzerResults::One;
            mResults->AddMarker(location, marker, mSettings->mInputChannel);
        }

        AnalyzerResults::MarkerType parity_marker = (parity_bit.first == parity) ? AnalyzerResults::X : AnalyzerResults::ErrorX;
        mResults->AddMarker(parity_bit.second + mT, parity_marker, mSettings->mInputChannel);
        AnalyzerResults::MarkerType stop_marker = (stop_bit.first == 1) ? AnalyzerResults::Stop : AnalyzerResults::ErrorX;
        mResults->AddMarker(stop_bit.second + mT, stop_marker, mSettings->mInputChannel);

        Frame   frame;
        FrameV2 frame_v2;
        frame.mStartingSampleInclusive = start_bit.second;
        frame.mEndingSampleInclusive   = location + (mT * 2);
        frame.mData1                   = byte;
        frame.mData2                   = mPacketByteCount;
        mResults->AddFrame(frame);
        frame_v2.AddInteger("data", byte);
        frame_v2.AddInteger("packet_byte", mPacketByteCount);
        mResults->AddFrameV2(frame_v2, "data", frame.mStartingSampleInclusive, frame.mEndingSampleInclusive);
        mResults->CommitResults();
        ReportProgress(mQi->GetSampleNumber());

        mPacketByteCount++;
        mBitsForNextByte.clear();
    }
}

bool QiAnalyzer::NeedsRerun() {
    return false;
}

U32 QiAnalyzer::GenerateSimulationData(U64                           minimum_sample_index,
                                       U32                           device_sample_rate,
                                       SimulationChannelDescriptor** simulation_channels) {
    if (mSimulationInitilized == false) {
        mSimulationDataGenerator.Initialize(GetSimulationSampleRate(), mSettings.get());
        mSimulationInitilized = true;
    }

    return mSimulationDataGenerator.GenerateSimulationData(minimum_sample_index, device_sample_rate, simulation_channels);
}

U32 QiAnalyzer::GetMinimumSampleRateHz() {
    return kBitRate * 8;
}

const char* QiAnalyzer::GetAnalyzerName() const {
    return ::GetAnalyzerName();
}

const char* GetAnalyzerName() {
    return "WPC Qi LLA";
}

Analyzer* CreateAnalyzer() {
    return new QiAnalyzer();
}

void DestroyAnalyzer(Analyzer* analyzer) {
    delete analyzer;
}
