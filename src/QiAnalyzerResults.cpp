#include "QiAnalyzerResults.h"
#include <AnalyzerHelpers.h>
#include "QiAnalyzer.h"
#include "QiAnalyzerSettings.h"
#include <iostream>
#include <fstream>

static U64 reverseDataByteBits(U64 data) {
    const U32 bit_count = 11;

    U64 byte = 0;
    for (U32 i = 0; i < bit_count; i++) {
        U64 lsb_val = (data & (1ull << i)) >> i;
        U64 msb_val = lsb_val << ((bit_count - 1) - i);
        byte |= msb_val;
    }

    return byte;
}

QiAnalyzerResults::QiAnalyzerResults(QiAnalyzer* analyzer, QiAnalyzerSettings* settings)
    : AnalyzerResults()
    , mSettings(settings)
    , mAnalyzer(analyzer) {}

QiAnalyzerResults::~QiAnalyzerResults() {}

void QiAnalyzerResults::GenerateBubbleText(U64 frame_index, Channel& channel, DisplayBase display_base) {
    ClearResultStrings();
    Frame frame = GetFrame(frame_index);

    U64  byte = reverseDataByteBits(frame.mData1);
    char number_str[32];
    AnalyzerHelpers::GetNumberString(byte, display_base, 11, number_str, 32);

    U64  packet_byte = frame.mData2;
    char packet_byte_str[16];
    AnalyzerHelpers::GetNumberString(packet_byte, Decimal, 8, packet_byte_str, 16);

    AddResultString(number_str, packet_byte_str);
}

void QiAnalyzerResults::GenerateExportFile(const char* file, DisplayBase display_base, U32 export_type_user_id) {
    std::ofstream file_stream(file, std::ios::out);

    U64 trigger_sample = mAnalyzer->GetTriggerSample();
    U32 sample_rate    = mAnalyzer->GetSampleRate();

    file_stream << "Time [s],Value,Packet Byte" << std::endl;

    U64 num_frames = GetNumFrames();
    for (U32 i = 0; i < num_frames; i++) {
        Frame frame = GetFrame(i);

        char time_str[128];
        AnalyzerHelpers::GetTimeString(frame.mStartingSampleInclusive, trigger_sample, sample_rate, time_str, 128);

        U64  byte = reverseDataByteBits(frame.mData1);
        char number_str[32];
        AnalyzerHelpers::GetNumberString(byte, display_base, 11, number_str, 32);

        U64  packet_byte = frame.mData2;
        char packet_byte_str[16];
        AnalyzerHelpers::GetNumberString(packet_byte, Decimal, 8, packet_byte_str, 16);

        file_stream << time_str << "," << number_str << "," << packet_byte_str << std::endl;

        if (UpdateExportProgressAndCheckForCancel(i, num_frames) == true) {
            file_stream.close();
            return;
        }
    }

    file_stream.close();
}

void QiAnalyzerResults::GenerateFrameTabularText(U64 frame_index, DisplayBase display_base) {
#ifdef SUPPORTS_PROTOCOL_SEARCH
    Frame frame = GetFrame(frame_index);
    ClearTabularText();

    U64  byte = reverseDataByteBits(frame.mData1);
    char number_str[32];
    AnalyzerHelpers::GetNumberString(byte, display_base, 11, number_str, 32);

    U64  packet_byte = frame.mData2;
    char packet_byte_str[16];
    AnalyzerHelpers::GetNumberString(packet_byte, Decimal, 8, packet_byte_str, 16);

    AddTabularText(number_str, packet_byte_str);
#endif
}

void QiAnalyzerResults::GeneratePacketTabularText(U64 packet_id, DisplayBase display_base) {
    // not supported
}

void QiAnalyzerResults::GenerateTransactionTabularText(U64 transaction_id, DisplayBase display_base) {
    // not supported
}