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
    Q_INVOKABLE void play();
    void convertToFlac();

signals:

public slots:
    void cppSlot(int number);
    void replyFinished(QNetworkReply* reply);
    void stopRecording();
    void stateChanged(QAudio::State newState);

private:
    QString parseReply(const QByteArray& response_body);
    QFile outputFile;
    QAudioInput* audio;
    QAudioFormat format;
    SF_INFO *sfinfo_in, *sfinfo_out;
    QString flacFilePath;
};

#endif // MYENGINE_H

