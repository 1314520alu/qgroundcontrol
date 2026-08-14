#pragma once

#include <chrono>
#include <optional>

#include <QtCore/QHash>
#include <QtCore/QLocale>
#include <QtCore/QObject>

class QTextToSpeech;
class Fact;
class AudioOutputTest;

/// \brief The AudioOutput class provides functionality for audio output using text-to-speech.
///
class AudioOutput : public QObject
{
    Q_OBJECT

    friend AudioOutputTest;

public:
    /// Enumeration for text modification options.
    enum class TextMod {
        None = 0,
        Translate = 1 << 0,
    };
    Q_FLAG(TextMod)
    Q_DECLARE_FLAGS(TextMods, TextMod)

    /// Constructs an AudioOutput object.
    ///     @param parent The parent QObject.
    explicit AudioOutput(QObject *parent = nullptr);

    /// Destructor for the AudioOutput class.
    ~AudioOutput();

    /// Gets the singleton instance of AudioOutput.
    ///     @return The singleton instance.
    static AudioOutput *instance();

    /// Initialize the Singleton
    ///     @param localeFact Optional TTS language Fact (`system` or a locale tag such as `zh_CN`).
    void init(Fact *volumeFact, Fact *mutedFact, Fact *localeFact = nullptr);

    /// Reads the specified text with optional text modifications.
    ///     @param text The text to be read.
    ///     @param textMods The text modifications to apply.
    void say(const QString &text, TextMods textMods = TextMod::None);

    /// Tests the audio output. Will stop current output before test
    void testAudioOutput();

private:
    QTextToSpeech *_engine = nullptr;
    qsizetype _textQueueSize = 0;
    bool _initialized = false;
    bool _speakCapable = false;
    Fact *_volumeFact = nullptr;
    Fact *_mutedFact = nullptr;
    Fact *_localeFact = nullptr;
    double _lastVolume = -1.0;
    QLocale _spokenLocale{QLocale::English, QLocale::UnitedStates};

    /// Returns the current volume (0.0 - 100.0) from the settings Fact.
    double _volumeSetting() const;

    /// Returns the current muted state from the settings Fact.
    bool _mutedSetting() const;

    /// Sets the TTS engine volume from the current Fact value.
    void _setVolume();

    /// Applies engine-dependent settings (locale, voice, cached capabilities) for the current engine.
    void _applyEngineSettings();

    /// Locale requested by the audio language setting (`system` uses the application locale).
    QLocale _requestedLocale() const;

    /// Best matching locale from the TTS engine.
    std::optional<QLocale> _bestAvailableLocale(const QLocale &wanted) const;

    /// Prefer a neural/natural voice when the engine exposes one for the current locale.
    void _selectNaturalVoice();

    /// Slightly slower rate for CJK voices; default otherwise.
    void _applySpeechRate();

    /// Finalizes initialization once the engine is Ready: applies settings, wires Facts, sets volume.
    void _finishInit();

    static const QHash<QString, QString> _textHash;

    static constexpr qsizetype kMaxTextQueueSize = 20;

    /// Grace period for async TTS backends (e.g. Android service binding) to reach Ready before warning.
    static constexpr std::chrono::seconds kEngineInitWarnTimeout{10};

    /// Fixes text messages for audio output.
    ///     @param string The text message to fix.
    ///     @param locale Locale used for spoken number/unit words.
    ///     @return The fixed text message.
    static QString _fixTextMessageForAudio(const QString &string,
                                           const QLocale &locale = QLocale(QLocale::English, QLocale::UnitedStates));

    /// Replaces predefined abbreviations with their corresponding full forms.
    static QString _replaceAbbreviations(const QString &input, const QLocale &locale);

    /// Replaces negative signs with the spoken negative word.
    static QString _replaceNegativeSigns(const QString &input, const QString &negativeWord);

    /// Replaces decimal points with the spoken decimal word.
    static QString _replaceDecimalPoints(const QString &input, const QString &pointWord);

    /// Replaces "m" (meters) with the spoken meters word following numbers.
    static QString _replaceMeters(const QString &input, const QString &metersWord);

    /// Converts millisecond values to a more readable format (seconds and minutes).
    static QString _convertMilliseconds(const QString &input, const QLocale &locale);

    /// Extracts a millisecond value from the given string.
    ///     @param string The string to extract from.
    ///     @param match The extracted millisecond string.
    ///     @param number The extracted number.
    ///     @return True if extraction is successful, false otherwise.
    static bool _getMillisecondString(const QString &string, QString &match, int &number);

};
Q_DECLARE_OPERATORS_FOR_FLAGS(AudioOutput::TextMods)
