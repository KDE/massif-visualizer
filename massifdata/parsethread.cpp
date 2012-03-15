/*
   This file is part of Massif Visualizer

   Copyright 2012 Milian Wolff <mail@milianw.de>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of
   the License or (at your option) version 3 or any later version
   accepted by the membership of KDE e.V. (or its successor approved
   by the membership of KDE e.V.), which shall act as a proxy
   defined in Section 14 of version 3 of the license.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "parsethread.h"
#include "parser.h"
#include "filedata.h"

#include <KUrl>
#include <KFilterDev>
#include <KIO/NetAccess>
#include <KTemporaryFile>
#include <KLocalizedString>
#include <KMessageBox>

namespace Massif {

ParseThread::ParseThread(QObject* parent)
: QThread(parent)
{

}

void ParseThread::startParsing(const KUrl& url, const QStringList& allocators)
{
    Q_ASSERT(m_url.isEmpty());
    m_url = url;
    m_allocators = allocators;
    start();
}

void ParseThread::run()
{
    QString file;
    if (!m_url.isLocalFile()) {
        if (!KIO::NetAccess::download(m_url, file, 0)) {
            setError(i18n("Download Failed"),
                     i18n("Failed to download remove massif data file <i>%1</i>.", m_url.pathOrUrl()));
            return;
        }
    } else {
        file = m_url.toLocalFile();
    }

    QScopedPointer<QIODevice> device(KFilterDev::deviceForFile(file));
    if (!device->open(QIODevice::ReadOnly)) {
        setError(i18n("Read Failed"),
                 i18n("Could not open file <i>%1</i> for reading.", file));
        return;
    }

    Parser p;
    QScopedPointer<FileData> data(p.parse(device.data(), m_allocators, &m_shouldStop));

    if (!data) {
        setError(i18n("Parser Failed"),
                 i18n("Could not parse file <i>%1</i>.<br>"
                      "Parse error in line %2:<br>%3",
                      m_url.pathOrUrl(), p.errorLine() + 1,
                      QString::fromLatin1(p.errorLineString())));
        return;
    } else if (data->snapshots().isEmpty()) {
        setError(i18n("Empty data file <i>%1</i>.", m_url.pathOrUrl()), i18n("Empty Data File"));
        return;
    }

    // success!
    emit finished(this, data.take());
}

void ParseThread::showError(QWidget* parent) const
{
    KMessageBox::error(parent, m_errorBody, m_errorTitle);
}

void ParseThread::stop()
{
    m_shouldStop = 1;
}

KUrl ParseThread::file() const
{
    return m_url;
}

void ParseThread::setError ( const QString& title, const QString& body )
{
    m_errorTitle = title;
    m_errorBody = body;
    emit finished(this, 0);
}

}

#include "parsethread.moc"
