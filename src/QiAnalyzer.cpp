#include <sstream>
#include <ios>
#include <algorithm>

#include "QiAnalyzer.h"
#include "QiAnalyzerSettings.h"
#include <AnalyzerChannelData.h>


#define CLAMP_MIN(VAL, MIN_VAL)     ((VAL) < (MIN_VAL) ? (MIN_VAL) : (VAL))


static const U32 kBitRate = 2000;


QiAnalyzer::QiAnalyzer()
    : Analyzer2()
    , mSettings(new QiAnalyzerSettings())
    , mQi(nullptr)
    , mSimulationInitilized(false)
    , mSampleRateHz(0)
    , mTLong(0)
    , mTShort(0)
    , mTLongMinError(0)
    , mTLongMaxError(0)
    , mTShortMinError(0)
    , mTShortMaxError(0)
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

    double period = 1.0 / double(kBitRate);
    double half_period = period / 2.0;
    // Period is 500uS
    mTLong = U32(mSampleRateHz * period);
    // Half a period, 250uS
    mTShort = U32(mSampleRateHz * half_period);
    // Pulse tolerances (long / short pulses)
    // 2:  50%          (250 / 125)
    // 3:  33.33%       (166 /  83)
    // 4:  25%          (125 /  62)
    // 5:  20%          (100 /  50)
    // 6:  16.67%       ( 83 /  41)
    // 8:  12.5%        ( 62 /  31)
    // 10: 10%          ( 50 /  25)
    mTLongMinError = CLAMP_MIN(mTLong / 4, 3);
    mTLongMaxError = CLAMP_MIN(mTLong / 4, 3);
    // Short min/max are only used for the initial pulse of a 1-bit; Long min/max are used for the whole bit
    mTShortMinError = CLAMP_MIN(mTShort / 2, 3);
    mTShortMaxError = CLAMP_MIN(mTShort / 2, 3);

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

U64 QiAnalyzer::AdvanceToNextEdge(U64 edge_location, U64* p_next_edge_location, U64* p_next_edge_distance) {
    U64 starting_edge_location = edge_location;

    mQi->AdvanceToNextEdge();
    U64 next_edge_location = mQi->GetSampleNumber();

    U64 next_edge_distance = next_edge_location - edge_location;

    while (((next_edge_distance <= (mTShort - mTShortMinError)) ||
            (next_edge_distance >= (mTShort + mTShortMaxError)))
                &&
           ((next_edge_distance <= (mTLong - mTLongMinError)) ||
            (next_edge_distance >= (mTLong + mTLongMaxError)))) {
        // We need two glitch edges as this is a differential signal, so the signal should bounce back to our expected state
        // mQi->AdvanceToNextEdge();
        // edge_location = mQi->GetSampleNumber();

        edge_location = next_edge_location;

        mQi->AdvanceToNextEdge();
        next_edge_location = mQi->GetSampleNumber();

        next_edge_distance = next_edge_location - edge_location;
    }

    *p_next_edge_location = next_edge_location;
    *p_next_edge_distance = next_edge_distance;

    return edge_location - starting_edge_location;
}

void QiAnalyzer::SynchronizeQiData() {
    while (mSynchronized == false) {
        CheckIfThreadShouldExit();

        U64 edge_location = mQi->GetSampleNumber();

        U64 next_edge_location, next_edge_distance;
        U64 skipped = AdvanceToNextEdge(edge_location, &next_edge_location, &next_edge_distance);
        // if (skipped > 0) {
        //     mResults->AddMarker(edge_location, AnalyzerResults::UpArrow, mSettings->mInputChannel);
        // }
        edge_location += skipped;

		// BitState next_bit_state = mQi->GetBitState();
		// BitState bit_state = (next_bit_state == BIT_LOW) ? BIT_HIGH : BIT_LOW;

		// if (bit_state == BIT_HIGH) {
			if ((next_edge_distance > (mTShort - mTShortMinError)) && (next_edge_distance < (mTShort + mTShortMaxError))) {
				// short
				U64 next_next_edge_location, next_next_edge_distance;
				skipped = AdvanceToNextEdge(next_edge_location, &next_next_edge_location, &next_next_edge_distance);
                // if (skipped > 0) {
                //     mResults->AddMarker(next_edge_location, AnalyzerResults::UpArrow, mSettings->mInputChannel);
                // }
                next_edge_location += skipped;
                next_edge_distance += skipped;

				if (((next_edge_distance + next_next_edge_distance) > (mTLong - mTLongMinError)) && ((next_edge_distance + next_next_edge_distance) < (mTLong + mTLongMaxError))) {
					// short again -> 1-bit
					mPreambleEdges.push_back(edge_location);
					mPreambleEdges.push_back(next_next_edge_location);

					mResults->AddMarker(edge_location, AnalyzerResults::Dot, mSettings->mInputChannel);
                } else if ((next_next_edge_distance > (mTLong - mTLongMinError)) && (next_next_edge_distance < (mTLong + mTLongMaxError))) {
    				// long -> 0-bit, so we must've synched on the second pulse of the 1-bits
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

                        // Mark the mis-synched edge as bad
                        mResults->AddMarker(next_edge_location, AnalyzerResults::ErrorDot, mSettings->mInputChannel);

                        // Record the second bit (the start bit). ProcessQuData() will continue from the edge of the first data bit.
                        SaveBit(next_edge_location, next_next_edge_location, 0);
                    } else {
                        // back to idle.
                        mResults->AddMarker(edge_location, AnalyzerResults::ErrorDot, mSettings->mInputChannel);
                        Invalidate();
                    }
				} else {
                    // back to idle.
                    mResults->AddMarker(edge_location, AnalyzerResults::ErrorDot, mSettings->mInputChannel);
                    Invalidate();
				}
			} else if ((next_edge_distance > (mTLong - mTLongMinError)) && (next_edge_distance < (mTLong + mTLongMaxError))) {
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
					SaveBit(edge_location, next_edge_location, 0);
				} else {
					// back to idle.
					mResults->AddMarker(edge_location, AnalyzerResults::ErrorX, mSettings->mInputChannel);
					Invalidate();
				}

				break;
			} else {
				// back to idle.
				mResults->AddMarker(edge_location, AnalyzerResults::ErrorSquare, mSettings->mInputChannel);
				Invalidate();
			}
		// } else {
		// 	// the first edge is always low->high, so we ignore any high->low transitions while synchronizing
		// 	mResults->AddMarker(edge_location, AnalyzerResults::DownArrow, mSettings->mInputChannel);
		// 	Invalidate();
		// }
    }
}

void QiAnalyzer::ProcessQiData() {
    if (mSynchronized == true) {
        // We're on the clock edge of a data, parity, or stop bit, as the start bit is recorded by SynchronizeQiData().
        U64 edge_location = mQi->GetSampleNumber();

        U64 next_edge_location, next_edge_distance;
        U64 skipped = AdvanceToNextEdge(edge_location, &next_edge_location, &next_edge_distance);
        next_edge_distance += skipped;

        if ((next_edge_distance > (mTShort - mTShortMinError)) && (next_edge_distance < (mTShort + mTShortMaxError))) {
            // short
            U64 next_next_edge_location, next_next_edge_distance;
            skipped = AdvanceToNextEdge(next_edge_location, &next_next_edge_location, &next_next_edge_distance);
            next_edge_location += skipped;
            next_edge_distance += skipped;

            if (((next_edge_distance + next_next_edge_distance) > (mTLong - mTLongMinError)) && ((next_edge_distance + next_next_edge_distance) < (mTLong + mTLongMaxError))) {
                // short again -> 1-bit
                SaveBit(edge_location, next_next_edge_location, 1);
            } else {
                // not synced anymore.
                mResults->AddMarker(edge_location, AnalyzerResults::ErrorDot, mSettings->mInputChannel);
                Invalidate();
                return;
            }
        } else if ((next_edge_distance > (mTLong - mTLongMinError)) && (next_edge_distance < (mTLong + mTLongMaxError))) {
            // long -> 0-bit
            SaveBit(edge_location, next_edge_location, 0);
        } else {
            // not synced anymore.
            mResults->AddMarker(edge_location, AnalyzerResults::ErrorSquare, mSettings->mInputChannel);
            Invalidate();
            return;
        }
    }
}

void QiAnalyzer::SaveBit(U64 location_start, U64 location_end, U32 value) {
    BitInfo info;
    info.start = location_start;
    info.end = location_end;
    info.value = value;
    mBitsForNextByte.push_back(info);

    const U32 bit_count = 11;
    if (mBitsForNextByte.size() == bit_count) {
        U64 packet = 0;
        for (U32 i = 0; i < mBitsForNextByte.size(); i++) {
            BitInfo& bit = mBitsForNextByte[i];

            U64 value = bit.value;
            packet |= value << i;
        }

        // Remove the start, parity, and stop bits so that we're left with just the data bits
        BitInfo& start_bit = mBitsForNextByte.front();
        mBitsForNextByte.pop_front();
        BitInfo& stop_bit = mBitsForNextByte.back();
        mBitsForNextByte.pop_back();
        BitInfo& parity_bit = mBitsForNextByte.back();
        mBitsForNextByte.pop_back();

        U8 byte = 0;
        for (U32 i = 0; i < mBitsForNextByte.size(); i++) {
            BitInfo& bit = mBitsForNextByte[i];

            U8 value = U8(bit.value);
            byte |= value << i;
        }

        U64 ones = 0;
        for (U32 i = 0; i < mBitsForNextByte.size(); i++) {
            BitInfo& bit = mBitsForNextByte[i];

            U64 value = bit.value;
            ones += value;
        }
        U32 parity = 1 - (ones % 2);

        mResults->AddMarker(start_bit.start + (start_bit.end - start_bit.start) / 2, AnalyzerResults::Start, mSettings->mInputChannel);

        for (U32 i = 0; i < mBitsForNextByte.size(); i++) {
            BitInfo& bit = mBitsForNextByte[i];

            U64 value    = bit.value;
            U64 location = bit.start + (bit.end - bit.start) / 2;

            AnalyzerResults::MarkerType marker = (value == 0) ? AnalyzerResults::Zero : AnalyzerResults::One;
            mResults->AddMarker(location, marker, mSettings->mInputChannel);
        }

        AnalyzerResults::MarkerType parity_marker = (parity_bit.value == parity) ? AnalyzerResults::X : AnalyzerResults::ErrorX;
        mResults->AddMarker(parity_bit.start + (parity_bit.end - parity_bit.start) / 2, parity_marker, mSettings->mInputChannel);
        AnalyzerResults::MarkerType stop_marker = (stop_bit.value == 1) ? AnalyzerResults::Stop : AnalyzerResults::ErrorX;
        mResults->AddMarker(stop_bit.start + (stop_bit.end - stop_bit.start) / 2, stop_marker, mSettings->mInputChannel);

        Frame   frame;
        FrameV2 frame_v2;
        frame.mStartingSampleInclusive = start_bit.start;
        frame.mEndingSampleInclusive   = stop_bit.end - 1;  // -1 as bits share an edge and the frame start/end ranges are inclusive and cannot overlap between frames
        frame.mData1                   = packet;
        frame.mData2                   = mPacketByteCount;
        mResults->AddFrame(frame);
        frame_v2.AddInteger("packet", packet);
        frame_v2.AddByte("payload", byte);
        frame_v2.AddByte("packet_byte", U8(mPacketByteCount));
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
    return kBitRate * 2 * 10;   // 10% error of a half period (for the short pulse of a 1-bit)
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
