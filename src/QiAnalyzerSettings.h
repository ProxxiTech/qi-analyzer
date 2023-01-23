#ifndef QI_ANALYZER_SETTINGS
#define QI_ANALYZER_SETTINGS

#include <AnalyzerSettings.h>
#include <AnalyzerTypes.h>

class QiAnalyzerSettings : public AnalyzerSettings {
  public:
    QiAnalyzerSettings();
    virtual ~QiAnalyzerSettings();

    virtual bool        SetSettingsFromInterfaces();
    virtual void        LoadSettings(const char* settings);
    virtual const char* SaveSettings();

    void UpdateInterfacesFromSettings();

    Channel mInputChannel;

  protected:
    std::unique_ptr<AnalyzerSettingInterfaceChannel> mInputChannelInterface;
};

#endif    // QI_ANALYZER_SETTINGS
