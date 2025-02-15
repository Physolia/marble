// SPDX-License-Identifier: LGPL-2.1-or-later
//
// SPDX-FileCopyrightText: 2006-2007 Torsten Rahn <tackat@kde.org>
// SPDX-FileCopyrightText: 2007 Inge Wallin <ingwa@kde.org>
// SPDX-FileCopyrightText: 2008, 2009 Jens-Michael Hoffmann <jensmh@gmx.de>
// SPDX-FileCopyrightText: 2008 Pino Toscano <pino@kde.org>
//

#include "HttpJob.h"

#include "HttpDownloadManager.h"
#include "MarbleDebug.h"

#include <QNetworkAccessManager>

using namespace Marble;

class Marble::HttpJobPrivate
{
public:
    HttpJobPrivate(const QUrl &sourceUrl, const QString &destFileName, const QString &id, QNetworkAccessManager *networkAccessManager);

    QUrl m_sourceUrl;
    QString m_destinationFileName;
    QString m_initiatorId;
    int m_trialsLeft;
    DownloadUsage m_downloadUsage;
    QString m_userAgent;
    QNetworkAccessManager *const m_networkAccessManager;
    QNetworkReply *m_networkReply;
};

HttpJobPrivate::HttpJobPrivate(const QUrl &sourceUrl, const QString &destFileName, const QString &id, QNetworkAccessManager *networkAccessManager)
    : m_sourceUrl(sourceUrl)
    , m_destinationFileName(destFileName)
    , m_initiatorId(id)
    , m_trialsLeft(3)
    , m_downloadUsage(DownloadBrowse)
    ,
    // FIXME: remove initialization depending on if empty pluginId
    // results in valid user agent string
    m_userAgent(QStringLiteral("unknown"))
    , m_networkAccessManager(networkAccessManager)
    , m_networkReply(nullptr)
{
}

HttpJob::HttpJob(const QUrl &sourceUrl, const QString &destFileName, const QString &id, QNetworkAccessManager *networkAccessManager)
    : d(new HttpJobPrivate(sourceUrl, destFileName, id, networkAccessManager))
{
}

HttpJob::~HttpJob()
{
    delete d;
}

QUrl HttpJob::sourceUrl() const
{
    return d->m_sourceUrl;
}

void HttpJob::setSourceUrl(const QUrl &url)
{
    d->m_sourceUrl = url;
}

QString HttpJob::initiatorId() const
{
    return d->m_initiatorId;
}

void HttpJob::setInitiatorId(const QString &id)
{
    d->m_initiatorId = id;
}

QString HttpJob::destinationFileName() const
{
    return d->m_destinationFileName;
}

void HttpJob::setDestinationFileName(const QString &fileName)
{
    d->m_destinationFileName = fileName;
}

bool HttpJob::tryAgain()
{
    if (d->m_trialsLeft > 0) {
        d->m_trialsLeft--;
        return true;
    } else {
        return false;
    }
}

DownloadUsage HttpJob::downloadUsage() const
{
    return d->m_downloadUsage;
}

void HttpJob::setDownloadUsage(const DownloadUsage usage)
{
    d->m_downloadUsage = usage;
}

void HttpJob::setUserAgentPluginId(const QString &pluginId) const
{
    d->m_userAgent = pluginId;
}

QByteArray HttpJob::userAgent() const
{
    switch (d->m_downloadUsage) {
    case DownloadBrowse:
        return HttpDownloadManager::userAgent(QStringLiteral("Browser"), d->m_userAgent);
    case DownloadBulk:
        return HttpDownloadManager::userAgent(QStringLiteral("BulkDownloader"), d->m_userAgent);
    default:
        qCritical() << "Unknown download usage value:" << d->m_downloadUsage;
        return HttpDownloadManager::userAgent(QStringLiteral("unknown"), d->m_userAgent);
    }
}

void HttpJob::execute()
{
    QNetworkRequest request(d->m_sourceUrl);
    request.setAttribute(QNetworkRequest::HttpPipeliningAllowedAttribute, true);
    request.setRawHeader("User-Agent", userAgent());
    d->m_networkReply = d->m_networkAccessManager->get(request);

    connect(d->m_networkReply, &QNetworkReply::downloadProgress, this, &HttpJob::downloadProgress);
    connect(d->m_networkReply, &QNetworkReply::errorOccurred, this, &HttpJob::error);
    connect(d->m_networkReply, &QNetworkReply::finished, this, &HttpJob::finished);
}
void HttpJob::downloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    Q_UNUSED(bytesReceived);
    Q_UNUSED(bytesTotal);
    //     mDebug() << "downloadProgress" << destinationFileName()
    //              << bytesReceived << '/' << bytesTotal;
}

void HttpJob::error(QNetworkReply::NetworkError code)
{
    mDebug() << "error" << destinationFileName() << code;
}

void HttpJob::finished()
{
    QNetworkReply::NetworkError const error = d->m_networkReply->error();
    //     mDebug() << "finished" << destinationFileName()
    //              << "error" << error;

    const QVariant httpPipeliningWasUsed = d->m_networkReply->attribute(QNetworkRequest::HttpPipeliningWasUsedAttribute);
    if (!httpPipeliningWasUsed.isNull())
        mDebug() << "http pipelining used:" << httpPipeliningWasUsed.toBool();

    switch (error) {
    case QNetworkReply::NoError: {
        // check if we are redirected
        const QVariant redirectionAttribute = d->m_networkReply->attribute(QNetworkRequest::RedirectionTargetAttribute);
        if (!redirectionAttribute.isNull()) {
            Q_EMIT redirected(this, redirectionAttribute.toUrl());
        } else {
            // no redirection occurred
            const QByteArray data = d->m_networkReply->readAll();
            Q_EMIT dataReceived(this, data);
        }
    } break;

    default:
        Q_EMIT jobDone(this, 1);
    }

    d->m_networkReply->disconnect(this);
    // No delete. This method is called by a signal QNetworkReply::finished.
    d->m_networkReply->deleteLater();
    d->m_networkReply = nullptr;
}

#include "moc_HttpJob.cpp"
