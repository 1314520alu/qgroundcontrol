#include "AudioOutput.h"
#include "Fact.h"
#include "AppMessages.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QRegularExpression>
#include <QtCore/QApplicationStatic>
#include <QtCore/QHash>
#include <QtCore/QLocale>
#include <QtCore/QTimer>
#include <QtTextToSpeech/QTextToSpeech>
#include <QtTextToSpeech/QVoice>

#include <algorithm>
#include <optional>

QGC_LOGGING_CATEGORY(AudioOutputLog, "Utilities.AudioOutput");
// qt.speech.tts.flite
// qt.speech.tts.android

const QHash<QString, QString> AudioOutput::_textHash = {
    { "ERR",            "error" },
    { "POSCTL",         "Position Control" },
    { "ALTCTL",         "Altitude Control" },
    { "AUTO_RTL",       "auto return to launch" },
    { "RTL",            "return To launch" },
    { "ACCEL",          "accelerometer" },
    { "RC_MAP_MODE_SW", "RC mode switch" },
    { "REJ",            "rejected" },
    { "WP",             "waypoint" },
    { "CMD",            "command" },
    { "COMPID",         "component eye dee" },
    { "PARAMS",         "parameters" },
    { "ID",             "I.D." },
    { "ADSB",           "A.D.S.B." },
    { "EKF",            "E.K.F." },
    { "PREARM",         "pre arm" },
    { "PITOT",          "pee toe" },
    { "SERVOX_FUNCTION","Servo X Function" },
};

namespace {

struct SpokenWords {
    QString negative;
    QString point;
    QString meters;
    QString second;
    QString seconds;
    QString minute;
    QString minutes;
    QString millisecond;
    QString testPhrase; // contains %1 for volume
};

SpokenWords spokenWordsFor(const QLocale &locale)
{
    switch (locale.language()) {
    case QLocale::Chinese:
        if (locale.territory() == QLocale::Taiwan || locale.territory() == QLocale::HongKong
            || locale.script() == QLocale::TraditionalChineseScript) {
            return {
                QStringLiteral("負 "),
                QStringLiteral(" 點 "),
                QStringLiteral(" 公尺"),
                QStringLiteral(" 秒"),
                QStringLiteral(" 秒"),
                QStringLiteral(" 分鐘"),
                QStringLiteral(" 分鐘"),
                QStringLiteral(" 毫秒"),
                QStringLiteral("語音測試。目前音量為百分之 %1"),
            };
        }
        return {
            QStringLiteral("负 "),
            QStringLiteral(" 点 "),
            QStringLiteral(" 米"),
            QStringLiteral(" 秒"),
            QStringLiteral(" 秒"),
            QStringLiteral(" 分钟"),
            QStringLiteral(" 分钟"),
            QStringLiteral(" 毫秒"),
            QStringLiteral("语音测试。当前音量为百分之 %1"),
        };
    case QLocale::Japanese:
        return {
            QStringLiteral("マイナス "),
            QStringLiteral(" てん "),
            QStringLiteral(" メートル"),
            QStringLiteral(" 秒"),
            QStringLiteral(" 秒"),
            QStringLiteral(" 分"),
            QStringLiteral(" 分"),
            QStringLiteral(" ミリ秒"),
            QStringLiteral("音声テスト。音量はパーセント %1 です"),
        };
    case QLocale::Korean:
        return {
            QStringLiteral("마이너스 "),
            QStringLiteral(" 점 "),
            QStringLiteral(" 미터"),
            QStringLiteral(" 초"),
            QStringLiteral(" 초"),
            QStringLiteral(" 분"),
            QStringLiteral(" 분"),
            QStringLiteral(" 밀리초"),
            QStringLiteral("음성 테스트. 현재 음량은 퍼센트 %1 입니다"),
        };
    case QLocale::Spanish:
        return {
            QStringLiteral("menos "),
            QStringLiteral(" punto "),
            QStringLiteral(" metros"),
            QStringLiteral(" segundo"),
            QStringLiteral(" segundos"),
            QStringLiteral(" minuto"),
            QStringLiteral(" minutos"),
            QStringLiteral(" milisegundo"),
            QStringLiteral("Prueba de audio. El volumen es %1 por ciento"),
        };
    case QLocale::French:
        return {
            QStringLiteral("moins "),
            QStringLiteral(" virgule "),
            QStringLiteral(" mètres"),
            QStringLiteral(" seconde"),
            QStringLiteral(" secondes"),
            QStringLiteral(" minute"),
            QStringLiteral(" minutes"),
            QStringLiteral(" milliseconde"),
            QStringLiteral("Test audio. Le volume est de %1 pour cent"),
        };
    case QLocale::German:
        return {
            QStringLiteral("minus "),
            QStringLiteral(" komma "),
            QStringLiteral(" meter"),
            QStringLiteral(" sekunde"),
            QStringLiteral(" sekunden"),
            QStringLiteral(" minute"),
            QStringLiteral(" minuten"),
            QStringLiteral(" millisekunde"),
            QStringLiteral("Audiotest. Die Lautstärke beträgt %1 Prozent"),
        };
    case QLocale::Russian:
        return {
            QStringLiteral("минус "),
            QStringLiteral(" точка "),
            QStringLiteral(" метров"),
            QStringLiteral(" секунда"),
            QStringLiteral(" секунд"),
            QStringLiteral(" минута"),
            QStringLiteral(" минут"),
            QStringLiteral(" миллисекунда"),
            QStringLiteral("Проверка звука. Громкость %1 процентов"),
        };
    case QLocale::Portuguese:
        return {
            QStringLiteral("menos "),
            QStringLiteral(" ponto "),
            QStringLiteral(" metros"),
            QStringLiteral(" segundo"),
            QStringLiteral(" segundos"),
            QStringLiteral(" minuto"),
            QStringLiteral(" minutos"),
            QStringLiteral(" milissegundo"),
            QStringLiteral("Teste de áudio. O volume é %1 por cento"),
        };
    case QLocale::Italian:
        return {
            QStringLiteral("meno "),
            QStringLiteral(" punto "),
            QStringLiteral(" metri"),
            QStringLiteral(" secondo"),
            QStringLiteral(" secondi"),
            QStringLiteral(" minuto"),
            QStringLiteral(" minuti"),
            QStringLiteral(" millisecondo"),
            QStringLiteral("Test audio. Il volume è %1 percento"),
        };
    case QLocale::Arabic:
        return {
            QStringLiteral("سالب "),
            QStringLiteral(" فاصلة "),
            QStringLiteral(" أمتار"),
            QStringLiteral(" ثانية"),
            QStringLiteral(" ثوان"),
            QStringLiteral(" دقيقة"),
            QStringLiteral(" دقائق"),
            QStringLiteral(" مللي ثانية"),
            QStringLiteral("اختبار الصوت. مستوى الصوت %1 بالمئة"),
        };
    default:
        return {
            QStringLiteral("negative "),
            QStringLiteral(" point "),
            QStringLiteral(" meters"),
            QStringLiteral(" second"),
            QStringLiteral(" seconds"),
            QStringLiteral(" minute"),
            QStringLiteral(" minutes"),
            QStringLiteral(" millisecond"),
            QStringLiteral("Audio test. Volume is %1 percent"),
        };
    }
}

int naturalVoiceScore(const QVoice &voice)
{
    const QString name = voice.name().toLower();
    int score = 0;
    if (name.contains(QLatin1String("neural"))
        || name.contains(QLatin1String("natural"))
        || name.contains(QLatin1String("wavenet"))
        || name.contains(QLatin1String("studio"))
        || name.contains(QLatin1String("journey"))
        || name.contains(QLatin1String("chirp"))
        || name.contains(QLatin1String("gemini"))) {
        score += 4;
    }
    if (name.contains(QLatin1String("network"))
        || name.contains(QLatin1String("online"))
        || name.contains(QLatin1String("premium"))
        || name.contains(QLatin1String("enhanced"))
        || name.contains(QLatin1String("google"))) {
        score += 2;
    }
    if (name.contains(QLatin1String("compact"))
        || name.contains(QLatin1String("pico"))
        || name.contains(QLatin1String("flite"))) {
        score -= 2;
    }
    if (voice.gender() == QVoice::Female) {
        score += 1;
    }
    return score;
}

} // namespace

Q_APPLICATION_STATIC(AudioOutput, _audioOutput);

AudioOutput::AudioOutput(QObject *parent)
    : QObject(parent)
    // Auto-select a real engine, except under unit tests where the "none" backend avoids probing
    // system plugins (e.g. speechd) that emit critical load errors and trip the strict log check.
    , _engine(QGC::runningUnitTests()
                  ? new QTextToSpeech(QStringLiteral("none"), this)
                  : new QTextToSpeech(this))
{
    // qCDebug(AudioOutputLog) << this;
}

AudioOutput::~AudioOutput()
{
    // qCDebug(AudioOutputLog) << this;
}

AudioOutput *AudioOutput::instance()
{
    return _audioOutput();
}

void AudioOutput::init(Fact* volumeFact, Fact* mutedFact, Fact* localeFact)
{
    Q_CHECK_PTR(volumeFact);
    Q_CHECK_PTR(mutedFact);

    if (_initialized) {
        return;
    }

    _volumeFact = volumeFact;
    _mutedFact = mutedFact;
    _localeFact = localeFact;

    // Some QTextToSpeech backends (notably Android) initialize asynchronously, so finalize on Ready rather than bailing (Qt docs).
    (void) connect(_engine, &QTextToSpeech::stateChanged, this, [this](QTextToSpeech::State state) {
        if (state == QTextToSpeech::State::Ready) {
            _textQueueSize = 0;
            if (!_initialized) {
                _finishInit();
            }
        }
        qCDebug(AudioOutputLog) << "TTS State changed to:" << state;
    });

    (void) connect(_engine, &QTextToSpeech::errorOccurred, this, [this](QTextToSpeech::ErrorReason reason, const QString &errorString) {
        qCWarning(AudioOutputLog) << "TTS error occurred. Reason:" << reason << ", Message:" << errorString;
        _textQueueSize = 0;
    });

    // Decrement as each utterance leaves the queue so the counter tracks live backlog, not cumulative enqueues since drain.
    (void) connect(_engine, &QTextToSpeech::aboutToSynthesize, this, [this](qsizetype) {
        if (_textQueueSize > 0) {
            _textQueueSize--;
        }
    });

    (void) connect(_engine, &QTextToSpeech::engineChanged, this, [this](const QString &engine) {
        qCDebug(AudioOutputLog) << "TTS Engine set to:" << engine;
        _applyEngineSettings();
    });

    if (_engine->state() == QTextToSpeech::State::Ready) {
        _finishInit();
    } else {
        // Some backends (notably Android) start in Error state while binding to the system TTS
        // service, then transition to Ready. Defer and only warn if the engine never becomes usable.
        qCDebug(AudioOutputLog) << "QTextToSpeech engine not ready; deferring init. State:" << _engine->state();
        if (!QGC::runningUnitTests()) {
            // Skip under unit tests: the "none" backend never becomes Ready and the warning would trip the strict log check.
            QTimer::singleShot(kEngineInitWarnTimeout, this, [this]() {
                if (!_initialized) {
                    qCWarning(AudioOutputLog) << "No usable QTextToSpeech engine available. Engine:" << _engine->engine()
                                              << "State:" << _engine->state()
                                              << "Reason:" << _engine->errorReason() << _engine->errorString();
                }
            });
        }
    }
}

void AudioOutput::_finishInit()
{
    _applyEngineSettings();

    (void) connect(_volumeFact, &Fact::valueChanged, this, [this]() {
        _setVolume();
    });

    (void) connect(_mutedFact, &Fact::valueChanged, this, [this]() {
        _setVolume();
    });

    if (_localeFact) {
        (void) connect(_localeFact, &Fact::valueChanged, this, [this]() {
            _applyEngineSettings();
        });
    }

    (void) connect(_engine, &QTextToSpeech::localeChanged, this, [this](const QLocale &locale) {
        qCDebug(AudioOutputLog) << "TTS Locale change to:" << locale;
        _spokenLocale = locale;
        _selectNaturalVoice();
        _applySpeechRate();
    });

    if (AudioOutputLog().isDebugEnabled()) {
        (void) connect(_engine, &QTextToSpeech::volumeChanged, this, [](double volume) {
            qCDebug(AudioOutputLog) << "TTS Volume changed to:" << volume;
        });
        (void) connect(_engine, &QTextToSpeech::sayingWord, this, [](const QString &word, qsizetype id, qsizetype start, qsizetype length) {
            qCDebug(AudioOutputLog) << "TTS Saying:" << word << "ID:" << id << "Start:" << start << "Length:" << length;
        });
    }

    _initialized = true;
    _setVolume();

    qCDebug(AudioOutputLog) << "AudioOutput initialized with volume:" << _volumeSetting() << "%";
}

double AudioOutput::_volumeSetting() const
{
    return std::clamp(_volumeFact->rawValue().toDouble(), 0.0, 100.0);
}

bool AudioOutput::_mutedSetting() const
{
    return _mutedFact->rawValue().toBool();
}

void AudioOutput::_applyEngineSettings()
{
    if (_engine->state() != QTextToSpeech::State::Ready) {
        return;
    }

    const QLocale wanted = _requestedLocale();
    if (const std::optional<QLocale> matched = _bestAvailableLocale(wanted)) {
        _spokenLocale = *matched;
        _engine->setLocale(*matched);
        qCDebug(AudioOutputLog) << "TTS locale set to" << *matched << "(requested" << wanted << ")";
    } else {
        _spokenLocale = _engine->locale();
        qCWarning(AudioOutputLog) << "No TTS voice for" << wanted
                                  << "- install a language pack in system TTS settings. Available:"
                                  << _engine->availableLocales();
    }

    _selectNaturalVoice();
    _applySpeechRate();

    _speakCapable = _engine->engineCapabilities().testFlag(QTextToSpeech::Capability::Speak);
}

QLocale AudioOutput::_requestedLocale() const
{
    const QString tag = _localeFact ? _localeFact->rawValue().toString() : QStringLiteral("system");
    if (tag.isEmpty() || tag == QLatin1String("system")) {
        return QLocale();
    }
    return QLocale(tag);
}

std::optional<QLocale> AudioOutput::_bestAvailableLocale(const QLocale &wanted) const
{
    const QList<QLocale> available = _engine->availableLocales();
    if (available.isEmpty()) {
        // Some backends (notably Android) populate locales only after setLocale.
        return wanted;
    }

    if (available.contains(wanted)) {
        return wanted;
    }

    for (const QLocale &locale : available) {
        if (locale.language() == wanted.language() && locale.territory() == wanted.territory()) {
            return locale;
        }
    }

    for (const QLocale &locale : available) {
        if (locale.language() == wanted.language()) {
            return locale;
        }
    }

    return std::nullopt;
}

void AudioOutput::_selectNaturalVoice()
{
    const QList<QVoice> voices = _engine->availableVoices();
    if (voices.isEmpty()) {
        return;
    }

    int bestIndex = 0;
    int bestScore = naturalVoiceScore(voices.constFirst());
    for (int i = 1; i < voices.size(); ++i) {
        const int score = naturalVoiceScore(voices.at(i));
        if (score > bestScore) {
            bestScore = score;
            bestIndex = i;
        }
    }

    _engine->setVoice(voices.at(bestIndex));
    qCDebug(AudioOutputLog) << "TTS voice set to" << voices.at(bestIndex).name() << "score" << bestScore;
}

void AudioOutput::_applySpeechRate()
{
    double rate = 0.0;
    switch (_spokenLocale.language()) {
    case QLocale::Chinese:
    case QLocale::Japanese:
    case QLocale::Korean:
        rate = -0.15;
        break;
    default:
        break;
    }
    _engine->setRate(rate);
}

void AudioOutput::_setVolume()
{
    const bool muted = _mutedSetting();
    const double volume = muted ? 0.0 : _volumeSetting();

    // qFuzzyCompare fails near zero; adding 1.0 shifts values into a safe range
    if (qFuzzyCompare(1.0 + volume, 1.0 + _lastVolume)) {
        return;
    }
    _lastVolume = volume;

    // Must normalize volume to 0.0 - 1.0 for QTextToSpeech
    const double normalizedVolume = volume / 100.0;
    (void) QMetaObject::invokeMethod(_engine, [this, volume, normalizedVolume]() {
        if (volume == 0.0) {
            // Prevent any queued text from being spoken once muted
            _engine->stop(QTextToSpeech::BoundaryHint::Immediate);
            _textQueueSize = 0;
        }
        _engine->setVolume(normalizedVolume);
    });
    qCDebug(AudioOutputLog) << "AudioOutput volume set to:" << volume << "%";
}

void AudioOutput::say(const QString &text, TextMods textMods)
{
    if (!_initialized) {
        if (!QGC::runningUnitTests()) {
            qCWarning(AudioOutputLog) << "AudioOutput not initialized. Call init() before using say().";
        }
        return;
    }

    if (_volumeSetting() <= 0.0 || _mutedSetting()) {
        return;
    }

    if (!_speakCapable) {
        qCWarning(AudioOutputLog) << "Speech Not Supported:" << text;
        return;
    }

    QString outText = _fixTextMessageForAudio(text, _spokenLocale);

    if (textMods.testFlag(TextMod::Translate)) {
        outText = tr("%1").arg(outText);
    }

    if (outText.isEmpty()) {
        return;
    }

    // All queue/counter mutation must stay on the engine thread (where stateChanged resets it).
    (void) QMetaObject::invokeMethod(_engine, [this, outText]() {
        if (_textQueueSize >= kMaxTextQueueSize) {
            _engine->stop(QTextToSpeech::BoundaryHint::Immediate);
            _textQueueSize = 0;
            qCWarning(AudioOutputLog) << "Text queue exceeded maximum size. Stopped current speech.";
        }

        const qsizetype index = _engine->enqueue(outText);
        if (index < 0) {
            qCWarning(AudioOutputLog) << "Failed to enqueue speech. State:" << _engine->state()
                                      << "Reason:" << _engine->errorReason();
            return;
        }

        _textQueueSize++;
        qCDebug(AudioOutputLog) << "Enqueued text with index:" << index << ", Queue Size:" << _textQueueSize;
    });
}

void AudioOutput::testAudioOutput()
{
    if (!_initialized) {
        qCWarning(AudioOutputLog) << "AudioOutput not initialized. Call init() before using testAudioOutput().";
        return;
    }

    // Main-thread only (QML-invoked): mutates the engine and counter directly without marshaling.
    _engine->stop(QTextToSpeech::BoundaryHint::Immediate);
    _textQueueSize = 0;

    const SpokenWords words = spokenWordsFor(_spokenLocale);
    const QString testText = words.testPhrase.arg(_volumeSetting(), 0, 'f', 1);
    say(testText);
}

QString AudioOutput::_fixTextMessageForAudio(const QString &string, const QLocale &locale)
{
    const SpokenWords words = spokenWordsFor(locale);
    QString result = string;
    result = _replaceAbbreviations(result, locale);
    result = _replaceNegativeSigns(result, words.negative);
    result = _replaceDecimalPoints(result, words.point);
    result = _replaceMeters(result, words.meters);
    result = _convertMilliseconds(result, locale);
    return result;
}

QString AudioOutput::_replaceAbbreviations(const QString &input, const QLocale &locale)
{
    static const QHash<QString, QString> chineseHash = {
        { "ERR",             QStringLiteral("错误") },
        { "POSCTL",          QStringLiteral("位置控制") },
        { "ALTCTL",          QStringLiteral("高度控制") },
        { "AUTO_RTL",        QStringLiteral("自动返航") },
        { "RTL",             QStringLiteral("返航") },
        { "ACCEL",           QStringLiteral("加速度计") },
        { "RC_MAP_MODE_SW",  QStringLiteral("遥控模式开关") },
        { "REJ",             QStringLiteral("拒绝") },
        { "WP",              QStringLiteral("航点") },
        { "CMD",             QStringLiteral("指令") },
        { "COMPID",          QStringLiteral("组件编号") },
        { "PARAMS",          QStringLiteral("参数") },
        { "ID",              QStringLiteral("编号") },
        { "ADSB",            QStringLiteral("A D S B") },
        { "EKF",             QStringLiteral("E K F") },
        { "PREARM",          QStringLiteral("解锁前检查") },
        { "PITOT",           QStringLiteral("空速管") },
        { "SERVOX_FUNCTION", QStringLiteral("舵机功能") },
    };
    const QHash<QString, QString> &table = (locale.language() == QLocale::Chinese) ? chineseHash : _textHash;

    QStringList words = input.split(' ');
    for (QString &word : words) {
        const auto it = table.constFind(word.toUpper());
        if (it != table.constEnd()) {
            word = it.value();
        }
    }

    return words.join(' ');
}

QString AudioOutput::_replaceNegativeSigns(const QString &input, const QString &negativeWord)
{
    static const QRegularExpression negNumRegex(QStringLiteral("-\\s*(?=\\d)"));
    Q_ASSERT(negNumRegex.isValid());

    QString output = input;
    (void) output.replace(negNumRegex, negativeWord);
    return output;
}

QString AudioOutput::_replaceDecimalPoints(const QString &input, const QString &pointWord)
{
    static const QRegularExpression realNumRegex(QStringLiteral("([0-9]+)(\\.)([0-9]+)"));
    Q_ASSERT(realNumRegex.isValid());

    QString output = input;
    QRegularExpressionMatch realNumRegexMatch = realNumRegex.match(output);
    while (realNumRegexMatch.hasMatch()) {
        if (!realNumRegexMatch.captured(2).isNull()) {
            (void) output.replace(realNumRegexMatch.capturedStart(2), realNumRegexMatch.capturedEnd(2) - realNumRegexMatch.capturedStart(2), pointWord);
        }
        realNumRegexMatch = realNumRegex.match(output);
    }

    return output;
}

QString AudioOutput::_replaceMeters(const QString &input, const QString &metersWord)
{
    static const QRegularExpression realNumMeterRegex(QStringLiteral("[0-9]*\\.?[0-9]\\s?(m)([^A-Za-z]|$)"));
    Q_ASSERT(realNumMeterRegex.isValid());

    QString output = input;
    QRegularExpressionMatch realNumMeterRegexMatch = realNumMeterRegex.match(output);
    while (realNumMeterRegexMatch.hasMatch()) {
        if (!realNumMeterRegexMatch.captured(1).isNull()) {
            (void) output.replace(realNumMeterRegexMatch.capturedStart(1), realNumMeterRegexMatch.capturedEnd(1) - realNumMeterRegexMatch.capturedStart(1), metersWord);
        }
        realNumMeterRegexMatch = realNumMeterRegex.match(output);
    }

    return output;
}

QString AudioOutput::_convertMilliseconds(const QString &input, const QLocale &locale)
{
    QString result = input;
    const SpokenWords words = spokenWordsFor(locale);

    QString match;
    int number;
    if (_getMillisecondString(input, match, number) && (number >= 1000)) {
        QString newNumber;
        if (number < 60000) {
            const int seconds = number / 1000;
            const int ms = number - (seconds * 1000);
            newNumber = QStringLiteral("%1%2").arg(seconds).arg(seconds == 1 ? words.second : words.seconds);
            if (ms > 0) {
                const QString connector = (locale.language() == QLocale::English) ? QStringLiteral(" and ") : QStringLiteral(" ");
                (void) newNumber.append(QStringLiteral("%1%2%3").arg(connector).arg(ms).arg(words.millisecond));
            }
        } else {
            const int minutes = number / 60000;
            const int seconds = (number - (minutes * 60000)) / 1000;
            newNumber = QStringLiteral("%1%2").arg(minutes).arg(minutes == 1 ? words.minute : words.minutes);
            if (seconds > 0) {
                const QString connector = (locale.language() == QLocale::English) ? QStringLiteral(" and ") : QStringLiteral(" ");
                (void) newNumber.append(QStringLiteral("%1%2%3").arg(connector).arg(seconds).arg(seconds == 1 ? words.second : words.seconds));
            }
        }
        (void) result.replace(match, newNumber);
    }

    return result;
}

bool AudioOutput::_getMillisecondString(const QString &string, QString &match, int &number)
{
    static const QRegularExpression msRegex("((?<number>[0-9]+)ms)");
    Q_ASSERT(msRegex.isValid());

    bool result = false;

    QRegularExpressionMatch regexpMatch = msRegex.match(string);
    if (regexpMatch.hasMatch()) {
        match = regexpMatch.captured(0);
        const QString numberStr = regexpMatch.captured("number");
        number = numberStr.toInt();
        result = true;
    }

    return result;
}
