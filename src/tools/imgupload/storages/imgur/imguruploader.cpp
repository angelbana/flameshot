// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2017-2019 Alejandro Sirgo Rica & Contributors

#include "imguruploader.h"
#include "utils/confighandler.h"
#include "utils/filenamehandler.h"
#include "utils/history.h"
#include "widgets/loadspinner.h"
#include "widgets/notificationwidget.h"

#include <QBuffer>
#include <QDesktopServices>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QShortcut>
#include <QUrlQuery>
#include <QVariant>

ImgurUploader::ImgurUploader(const QPixmap& capture, QWidget* parent)
  : ImgUploaderBase(capture, parent)
{
    m_NetworkAM = new QNetworkAccessManager(this);
    connect(m_NetworkAM,
            &QNetworkAccessManager::finished,
            this,
            &ImgurUploader::handleReply);
}

void ImgurUploader::handleReply(QNetworkReply* reply)
{
    spinner()->deleteLater();
    m_currentImageName.clear();
    QJsonDocument response = QJsonDocument::fromJson(reply->readAll());
    QJsonObject json = response.object();
    if (reply->error() == QNetworkReply::NoError) {
        QJsonObject data = json[QStringLiteral("data")].toObject();
        setImageURL(data[QStringLiteral("link")].toString());

        auto deleteToken = data[QStringLiteral("deletehash")].toString();

        // save history
        m_currentImageName = imageURL().toString();
        int lastSlash = m_currentImageName.lastIndexOf("/");
        if (lastSlash >= 0) {
            m_currentImageName = m_currentImageName.mid(lastSlash + 1);
        }

        // save image to history
        History history;
        m_currentImageName =
          history.packFileName("imgur", deleteToken, m_currentImageName);
        history.save(pixmap(), m_currentImageName);

        emit uploadOk(imageURL());
    } else {
        const int status =
          reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (status == 429) {
            setInfoLabelText(
              tr("Imgur temporarily rejected the upload (HTTP 429). "
                 "Try again later."));
        } else if (status == 0) {
            setInfoLabelText(tr("Could not connect to Imgur.") + "\n" +
                             reply->errorString());
        } else {
            QString message = tr("Imgur upload failed (HTTP %1).").arg(status);
            QString detail = json.value(QStringLiteral("data"))
                               .toObject()
                               .value(QStringLiteral("error"))
                               .toString()
                               .trimmed();
            if (detail.isEmpty()) {
                const QJsonArray errors =
                  json.value(QStringLiteral("errors")).toArray();
                if (!errors.isEmpty()) {
                    const QJsonObject error = errors.first().toObject();
                    detail = error.value(QStringLiteral("status")).toString();
                    const QString code = error.value(QStringLiteral("code"))
                                           .toVariant()
                                           .toString();
                    if (!code.isEmpty()) {
                        detail =
                          detail.isEmpty() ? code : code + " - " + detail;
                    }
                }
            }
            if (!detail.isEmpty()) {
                message += "\n" + detail;
            }
            setInfoLabelText(message);
        }
    }
    new QShortcut(Qt::Key_Escape, this, SLOT(close()));
}

void ImgurUploader::upload()
{
    QByteArray byteArray;
    QBuffer buffer(&byteArray);
    pixmap().save(&buffer, "PNG");

    QUrlQuery urlQuery;
    urlQuery.addQueryItem(QStringLiteral("title"), QStringLiteral(""));
    QString description = FileNameHandler().parsedPattern();
    urlQuery.addQueryItem(QStringLiteral("description"), description);

    QUrl url(QStringLiteral("https://api.imgur.com/3/image"));
    url.setQuery(urlQuery);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader,
                      "application/application/x-www-form-urlencoded");
    request.setRawHeader("Authorization",
                         QStringLiteral("Client-ID %1")
                           .arg(ConfigHandler().uploadClientSecret())
                           .toUtf8());

    m_NetworkAM->post(request, byteArray);
}

void ImgurUploader::deleteImage(const QString& fileName,
                                const QString& deleteToken)
{
    Q_UNUSED(fileName)
    bool successful = QDesktopServices::openUrl(
      QUrl(QStringLiteral("https://imgur.com/delete/%1").arg(deleteToken)));
    if (!successful) {
        notification()->showMessage(tr("Unable to open the URL."));
    }

    emit deleteOk();
}
