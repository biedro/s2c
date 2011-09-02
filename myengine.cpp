/*
** Copyright (C) 2011 Marcin Biedroñ
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation; either version 2.1 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU Lesser General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/


#include <QDebug>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QtMultimedia/QAudioInput>
//#include <QFile> //in header
#include <QFileInfo>
#include <QTimer>

#include <QtScript>

#include <stdio.h>
#include <stdlib.h>

#include "myengine.h"
#include "external/sndfile.h"

#include <phonon>


const char* const kHypothesesStr = "hypotheses";
const char* const kUtteranceStr = "utterance";
const char* const kConfidenceStr = "confidence";

MyEngine::MyEngine(QObject *parent) :
    QObject(parent)
{
    //not to record it every time :)
    flacFilePath = QString("C:/Users/marcin.biedron/demo_speech-build-desktop/test.flac");

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
    qDebug() << "Sending file: " << flacFilePath;
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

    QTimer::singleShot(2000, this, SLOT(stopRecording())); // Records audio for 2 sec
    audio->start(&outputFile);

}

void MyEngine::play() {
    qDebug() << "> Play()";
    //// move to different class
    //Phonon::MediaObject *music =
    //        Phonon::createPlayer(Phonon::MusicCategory,
     //                            Phonon::MediaSource("/path/mysong.mp3"));
    //music->play();

    QByteArray response_body("\"{\"status\":0,\"id\":\"debc3120d3cfe2e60fc43505b184883c-1\",\"hypotheses\":[{\"utterance\":\"system\",\"confidence\":0.6186791}]}");


    if (response_body.count() == 0) {
        qDebug() << "Response was empty.";
    }
    qDebug() << "Parsing response " << response_body;

    // Parse the response, ignoring comments.


    QScriptValue sc;
    QScriptEngine engine;
    sc = engine.evaluate(QString(response_body)); // In new versions it may need to look like engine.evaluate("(" + QString(result) + ")");

    if (sc.property(kHypothesesStr).isArray())
    {
        qDebug() << "jestem w domu";

        QStringList items;
        qScriptValueToSequence(sc.property(kHypothesesStr), items);

        foreach (QString str, items) {
            qDebug("value %s",str.toStdString().c_str());
        }

    } else {
        qDebug() << "fail";
    }

    qDebug() << "< Play()";
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
    //
    qDebug() << "Called replayFinished slot";
    reply->error()==QNetworkReply::NoError?qDebug() << "Error nr" << reply->error() : qDebug() << "no errors";
    QByteArray response = reply->readAll();
    parseReply(response); //
    reply->deleteLater();
}


QString MyEngine::parseReply(const QByteArray& response_body) {
    //looking for example pattern:
    //Error nr 0
    //response:
    //{"status":0,"id":"421d36084c097778b2f81d60caef6d99-1","hypotheses":[{"utterance":"system","confidence":0.6186791}]}

    if (response_body.count() == 0) {
        qDebug() << "Response was empty.";
        return QString();
    }
    qDebug() << "Parsing response " << response_body;

    // Parse the response, ignoring comments.


    QScriptValue sc;
    QScriptEngine engine;
    sc = engine.evaluate(QString(response_body)); // In new versions it may need to look like engine.evaluate("(" + QString(result) + ")");

    if (sc.property("kHypothesesStr").isArray())
    {

        QStringList items;
        qScriptValueToSequence(sc.property("response"), items);

        foreach (QString str, items) {
            qDebug("value %s",str.toStdString().c_str());
        }

    } else {
        qDebug() << "fail";
    }

    //    std::string error_msg;
    //    scoped_ptr<Value> response_value(base::JSONReader::ReadAndReturnError(
    //                                         response, false, NULL, &error_msg));
    //    if (response_value == NULL) {
    //        LOG(WARNING) << "ParseServerResponse: JSONReader failed : " << error_msg;
    //        return false;
    //    }

    //    if (!response_value->IsType(Value::TYPE_DICTIONARY)) {
    //        VLOG(1) << "ParseServerResponse: Unexpected response type "
    //                << response_value->GetType();
    //        return false;
    //    }
    //    const DictionaryValue* response_object =
    //            static_cast<DictionaryValue*>(response_value.get());

    //    // Get the hypotheses
    //    Value* hypotheses_value = NULL;
    //    if (!response_object->Get(kHypothesesString, &hypotheses_value)) {
    //        VLOG(1) << "ParseServerResponse: Missing hypotheses attribute.";
    //        return false;
    //    }
    //    DCHECK(hypotheses_value);
    //    if (!hypotheses_value->IsType(Value::TYPE_LIST)) {
    //        VLOG(1) << "ParseServerResponse: Unexpected hypotheses type "
    //                << hypotheses_value->GetType();
    //        return false;
    //    }
    //    const ListValue* hypotheses_list = static_cast<ListValue*>(hypotheses_value);

    //    size_t index = 0;
    //    for (; index < hypotheses_list->GetSize(); ++index) {
    //        Value* hypothesis = NULL;
    //        if (!hypotheses_list->Get(index, &hypothesis)) {
    //            LOG(WARNING) << "ParseServerResponse: Unable to read hypothesis value.";
    //            break;
    //        }
    //        DCHECK(hypothesis);
    //        if (!hypothesis->IsType(Value::TYPE_DICTIONARY)) {
    //            LOG(WARNING) << "ParseServerResponse: Unexpected value type "
    //                         << hypothesis->GetType();
    //            break;
    //        }

    //        const DictionaryValue* hypothesis_value =
    //                static_cast<DictionaryValue*>(hypothesis);
    //        string16 utterance;
    //        if (!hypothesis_value->GetString(kUtteranceString, &utterance)) {
    //            LOG(WARNING) << "ParseServerResponse: Missing utterance value.";
    //            break;
    //        }

    //        // It is not an error if the 'confidence' field is missing.
    //        double confidence = 0.0;
    //        hypothesis_value->GetDouble(kConfidenceString, &confidence);

    //        result->push_back(speech_input::SpeechInputResultItem(utterance,
    //                                                              confidence));
    //    }

    //    if (index < hypotheses_list->GetSize()) {
    //        result->clear();
    //        return false;
    //    }

    //    return true;
    return QString();
}
