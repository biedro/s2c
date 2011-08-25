#include <QDebug>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QtMultimedia/QAudioInput>
//#include <QFile> //in header
#include <QFileInfo>
#include <QTimer>

#include <stdio.h>
#include <stdlib.h>

#include "myengine.h"
#include "external/sndfile.h"


MyEngine::MyEngine(QObject *parent) :
    QObject(parent)
{
    // set up the format you want, eg.
    format.setFrequency(8000);
    format.setChannels(1);
    format.setSampleSize(8);
    format.setCodec("audio/pcm");
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::UnSignedInt);

    sfinfo_in = (SF_INFO *) malloc(sizeof(SF_INFO));
    sfinfo_out = (SF_INFO *) malloc(sizeof(SF_INFO));

    // out format will not change
    sfinfo_out->frames = 0;  //?
    sfinfo_out->samplerate = 8000;
    sfinfo_out->channels = 1;
    //sfinfo_out->format = SF_FORMAT_FLAC | SF_FORMAT_PCM_S8;
    sfinfo_out->format = SF_FORMAT_FLAC | SF_FORMAT_PCM_16;
    sfinfo_out->sections = 0;//?
    sfinfo_out->seekable = 0;//?

    // check if SF_INFO is correct
    sf_format_check(sfinfo_out)?qDebug() << "sfinfo_out is valid":qDebug() << "sfinfo_out is invalid";
}

MyEngine::~MyEngine()
{
    delete sfinfo_in;
    delete sfinfo_out;
}

void MyEngine::sendGoogleReq(const QString &msg) {
    qDebug() << "Called the sendGoogleReq method with " << msg;

    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    connect(manager, SIGNAL(finished(QNetworkReply*)),
            this, SLOT(replyFinished(QNetworkReply*)));

    //manager->get(QNetworkRequest(QUrl("http://qt.nokia.com")));
    //QNetworkRequest myReq = QNetworkRequest(QUrl("https://www.google.com/speech-api/v1/recognize?xjerr=1&client=chromium&lang=en-US"));
    // myReq.setHeader(QNetworkRequest::ContentTypeHeader, "audio/pcm; rate=8000");

    QNetworkRequest myReq = QNetworkRequest(QUrl("https://www.google.com/speech-api/v1/recognize?xjerr=1&client=test&lang=pl-PL"));
    myReq.setHeader(QNetworkRequest::ContentTypeHeader, "audio/x-flac; rate=8000");

    //req.setHeader(QNetworkRequest::ContentTypeHeader, "");
    //open audio file to send
    QFile file(flacFilePath);
    if (file.exists()) {
        file.open(QIODevice::ReadOnly)?qDebug() << "file opened: " << flacFilePath:qCritical() << "file open error: " << flacFilePath;
        QByteArray data = file.readAll();
        // myReq.setRawHeader("Content", data);
        manager->post(myReq, data);
    }

}

void MyEngine::recordVoice() {
    qDebug() << "Called recordVoice" ;
    QAudioDeviceInfo info = QAudioDeviceInfo::defaultInputDevice();
    foreach (const QString &str, info.supportedCodecs ())
        qDebug() << "Supported codecs: "<< str;

    foreach (const QAudioDeviceInfo &audioDeviceInfo, QAudioDeviceInfo::availableDevices ( QAudio::AudioInput ))
        qDebug() << "Devices: " << audioDeviceInfo.deviceName();

    outputFile.setFileName("test.raw");
    outputFile.open( QIODevice::WriteOnly | QIODevice::Truncate )?qDebug() << "file created":qDebug() << "file creation error";

    //qDebug() << "Sample rate: " << format.sampleRate();

    //QAudioDeviceInfo info = QAudioDeviceInfo::defaultInputDevice();
    if (!info.isFormatSupported(format)) {
        qWarning()<<"default format not supported try to use nearest";
        format = info.nearestFormat(format);
    }

    audio = new QAudioInput(format, this);
    connect(audio, SIGNAL(stateChanged(QAudio::State)),
            this, SLOT(stateChanged(QAudio::State)));

    QTimer::singleShot(3000, this, SLOT(stopRecording())); // Records audio for 3 sec
    audio->start(&outputFile);

}

void MyEngine::stopRecording()
{
    audio->stop();
    outputFile.close();
    delete audio;
    // output file saved as RAW
    // now convert to flac
    convertToFlac();
}

void MyEngine::convertToFlac()
{
    sfinfo_in->frames = 0;  //to override previous data
    sfinfo_in->samplerate = format.sampleRate();
    sfinfo_in->channels = format.channels();
    sfinfo_in->format = SF_FORMAT_RAW | SF_FORMAT_PCM_U8 | SF_ENDIAN_LITTLE;
    sfinfo_in->sections = 0;//to override previous data
    sfinfo_in->seekable = 0;//to override previous data

    sf_format_check(sfinfo_in)?qDebug() << "sfinfo_in is valid":qDebug() << "sfinfo_in is invalid";

    SNDFILE*  sf_file_in;

    qDebug() << "opening: " << QFile::encodeName(QFileInfo( outputFile ).absoluteFilePath());
    if( (sf_file_in = sf_open(
             QFile::encodeName(QFileInfo( outputFile ).absoluteFilePath()),
             SFM_READ,
             sfinfo_in)) == NULL) {
        qCritical() << "Error opening RAW file: " << sf_strerror( NULL );
    } else {
        qDebug() << "RAW file opened correctly";
    }

    qDebug() << "Data for " << QFileInfo( outputFile ).absoluteFilePath() <<
                " channels: " << sfinfo_in->channels <<
                " format: " << sfinfo_in->format <<
                " frames: " << sfinfo_in->frames <<
                " samplerate: " <<  sfinfo_in->samplerate <<
                " sections: " << sfinfo_in->sections <<
                " seekable: " <<  sfinfo_in->seekable;

    // delete old flac file
    flacFilePath = QFileInfo( outputFile ).absolutePath() + QString("/test.flac");
    QFile myFlacFile(flacFilePath);
    if (myFlacFile.exists()) {
        qDebug() << "deleting flac file";
        if (myFlacFile.remove()) {
            qDebug() << "file " << flacFilePath << "removed succesfully";
        } else {
            qCritical() << "file " << flacFilePath << "removing problem!";
        }
    } else {
        qDebug() << "no flac file";
    }

    // create flac file
    SNDFILE*  sf_file_out;
    if ((sf_file_out = sf_open ( QFile::encodeName(flacFilePath),
                                 SFM_WRITE,
                                 sfinfo_out)) == NULL){
        qDebug() << "Error opening/creating FLAC file: " << sf_strerror( NULL );
    } else {
        qDebug() << "FLAC file opened correctly";
    }

    // copy RAW to FLAC

    const int bufsize = 10000;
    short* buffer = new short[bufsize * sfinfo_in->channels];
    sf_count_t cnt = sfinfo_in->frames;
    while (cnt) {
        // libsndfile does the conversion for us (if needed)
        sf_count_t n = sf_readf_short(sf_file_in, buffer, bufsize);
        // write from buffer directly (physically) into .gig file
        sf_count_t m = sf_writef_short(sf_file_out, buffer, n);
        n==m?qDebug("allright"):qCritical("epicfail while converting");
        cnt -= n;
    }
    delete[] buffer;

    //close opened file
    sf_close(sf_file_in);
    sf_close(sf_file_out);

    // check if it opens correctly
/*
    SF_INFO sfinfo_chk;
    sf_format_check(&sfinfo_chk)?qDebug() << "sfinfo_in is valid":qDebug() << "sfinfo_in is invalid";
    SNDFILE*  sf_file_chk;
    if ((sf_file_chk = sf_open ( QFile::encodeName(flacFilePath),
                                 SFM_READ,
                                 &sfinfo_chk)) == NULL){
        qDebug() << "Check - Error opening/creating FLAC file: " << sf_strerror( NULL );
    } else {
        qDebug() << "Check - FLAC file opened correctly";
    }
    sf_format_check(&sfinfo_chk)?qDebug() << "sfinfo_in is valid":qDebug() << "sfinfo_in is invalid";

    qDebug() << "Data for " << QFileInfo( flacFilePath ).absoluteFilePath() <<
                "channels: " << sfinfo_chk.channels <<
                " format: " << sfinfo_chk.format <<
                " frames: " << sfinfo_chk.frames <<
                " samplerate: " <<  sfinfo_chk.samplerate <<
                " sections: " << sfinfo_chk.sections <<
                " seekable: " <<  sfinfo_chk.seekable;

    sf_close(sf_file_chk);
    */
}

void MyEngine::stateChanged(QAudio::State newState)
{
    switch(newState) {
    case QAudio::StoppedState:
        if (audio->error() != QAudio::NoError) {
            qDebug() << "Audio device stopped with error";
        } else {
            qDebug() << "Audio device stopped without errors";
        }
        break;
    default:
        qDebug() << "Audio device state Changed id:" << newState;

    }
}

void MyEngine::cppSlot(int number) {
    qDebug() << "Called the C++ slot with" << number;
}

void MyEngine::replyFinished(QNetworkReply* reply) {
    QByteArray response = reply->readAll();
    qDebug() << "Called replayFinished slot";
    reply->error()==QNetworkReply::NoError?qDebug() << "Error nr" << reply->error() : qDebug() << "no errors";
    qDebug() << "response:";
    qDebug() << response.data();
    reply->deleteLater();
}
