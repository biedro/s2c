#ifndef MYENGINE_H
#define MYENGINE_H

#include <QObject>
#include <QFile>
#include <QAudio>
#include <QAudioFormat>

class QString;
class QNetworkReply;
class QAudioInput;
class SF_INFO;

class MyEngine : public QObject
{
    Q_OBJECT
public:
    explicit MyEngine(QObject *parent = 0);
    virtual ~MyEngine();
    Q_INVOKABLE void sendGoogleReq(const QString &msg);
    Q_INVOKABLE void recordVoice();
    void convertToFlac();

signals:

public slots:
    void cppSlot(int number);
    void replyFinished(QNetworkReply* reply);
    void stopRecording();
    void stateChanged(QAudio::State newState);

private:
    QFile outputFile;
    QAudioInput* audio;
    QAudioFormat format;
    SF_INFO *sfinfo_in, *sfinfo_out;
    QString flacFilePath;
};

#endif // MYENGINE_H

