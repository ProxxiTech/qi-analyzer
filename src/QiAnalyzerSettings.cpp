#include "QiAnalyzerSettings.h"
#include <AnalyzerHelpers.h>

QiAnalyzerSettings::QiAnalyzerSettings() : mInputChannel(UNDEFINED_CHANNEL) {
    mInputChannelInterface.reset(new AnalyzerSettingInterfaceChannel());
    mInputChannelInterface->SetTitleAndTooltip("Qi", "WPC Qi");
    mInputChannelInterface->SetChannel(mInputChannel);

    AddInterface(mInputChannelInterface.get());

    AddExportOption(0, "Export as text/csv file");
    AddExportExtension(0, "text", "txt");
    AddExportExtension(0, "csv", "csv");

    ClearChannels();
    AddChannel(mInputChannel, "Qi", false);
}

QiAnalyzerSettings::~QiAnalyzerSettings() {}

bool QiAnalyzerSettings::SetSettingsFromInterfaces() {
    mInputChannel = mInputChannelInterface->GetChannel();

    ClearChannels();
    AddChannel(mInputChannel, "Qi", true);

    return true;
}

void QiAnalyzerSettings::LoadSettings(const char* settings) {
    SimpleArchive text_archive;
    text_archive.SetString(settings);

    const char* name_string;
    text_archive >> &name_string;
    if (strcmp(name_string, "QiAnalyzer") != 0)
        AnalyzerHelpers::Assert("QiAnalyzer: LoadSettings() called with a settings string from a different analyzer.");

    text_archive >> mInputChannel;

    ClearChannels();
    AddChannel(mInputChannel, "Qi", true);

    UpdateInterfacesFromSettings();
}

const char* QiAnalyzerSettings::SaveSettings() {
    SimpleArchive text_archive;

    text_archive << "QiAnalyzer";
    text_archive << mInputChannel;

    return SetReturnString(text_archive.GetString());
}

void QiAnalyzerSettings::UpdateInterfacesFromSettings() {
    mInputChannelInterface->SetChannel(mInputChannel);
}
