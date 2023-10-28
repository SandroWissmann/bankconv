#include "Backend.hpp"

#include <QtCore/QDir>
#include <QtCore/QProcess>
#include <QtCore/QRegularExpression>

#include <algorithm>

#include <QDebug>

namespace bankconv {

namespace {

struct EntryCheckingAccount{
    QString accountNumber; // Auftragskonto
    QString dateFirst; // Buchung -> first date
    QString dateSecond;// Valuata -> second date
    QString payee; // Auftragsgeber/Empfänger
    QString bookingType; // Buchungstext
    QString bookingReason; // Verwendungszweck
    QString saldo; // Saldo -> How much is on the account after booking
    QString currency{"EUR"}; // WÄhrung
    QString amount; // Betrag
    QString rawEntry;
};

std::vector<QString> toRows(
    const QByteArray& rawDataFromPdf)
{
    QString result = QString::fromUtf8(rawDataFromPdf);
    QStringList rawRows = result.split("\\n");
    qWarning() << rawRows.size();

    std::vector<QString> rows;
    rows.reserve(rawRows.size());
    for(auto& rawRow : rawRows) {
        //rawRow = rawRow.simplified();
        if(rawRow.isEmpty()) {
            continue;
        }
        rows.emplace_back(rawRow);
    }
    qWarning() << rows.size();
    return rows;
}

QString extractStartValue(const QString& row)
{
    auto startValue = row.section(' ', -1);
    // TODO validate
    return startValue;
}

QString extractFirstDate(const QString& row)
{
    // TODO error handling
    return row.section(' ', 0,0);
}

QString extractSecondDate(const QString& row)
{
    // TODO error handling
    return row.section(' ', 1,1);
}

QString extractBookingType(const QString& row)
{
    // TODO error handling
    return row.section(' ', 2,2);
}

bool isAmount(const QString& row)
{
    static QRegularExpression re{R"(\d+,\d{2}(\+|-))"};
    auto match = re.match(row);
    return match.hasMatch();
}

std::pair<QString, QString> extractPayeeAndBookingReason(
    std::vector<QString>::const_iterator& cit)
{
    const auto& entries = cit->split(QRegularExpression{R"(\s{2,})"});
    qWarning() << entries;
    Q_ASSERT(entries.size() == 2);
    const auto payee = entries[0];
    auto  bookingReason = entries[1];
    ++cit;
    while(!isAmount(*cit)) {
        if(!bookingReason.isEmpty()) {
            bookingReason += " ";
        }
        bookingReason += *cit;
        ++cit;
    }

    return {payee, bookingReason};
}

std::vector<EntryCheckingAccount> toEntriesCheckingAccount(
    const QByteArray& rawDataFromPdf)
{
    const auto rows = toRows(rawDataFromPdf);

    auto cit = std::find_if(rows.cbegin(), rows.cend(), [](const QString& row){
        return row.startsWith(" Kontostand");
    });
    const auto startValue = extractStartValue(*cit);
    qWarning() << "startValue: " << startValue;

    ++cit;
    const auto firstDate = extractFirstDate(*cit);
    qWarning() << "firstDate: " <<  firstDate;
    const auto secondDate = extractSecondDate(*cit);
    qWarning() << "secondDate: " <<  secondDate;
    const auto bookingType = extractBookingType(*cit);
    qWarning() << "bookingType: " <<  bookingType;
    ++cit;

    // todo: based on booking type extraction might need adjustment here
    const auto [payee,bookingReason ] = extractPayeeAndBookingReason(cit);
    qWarning() << "payee: " << bookingReason;
    qWarning() << "bookingReason: " << bookingReason;

    const auto amount = cit->simplified();
    qWarning() << amount;

    std::vector<EntryCheckingAccount> entries;
    entries.reserve(rows.size());
    for(const auto& row : rows) {
        EntryCheckingAccount entry;
        entry.rawEntry = row;
        entries.emplace_back(entry);
    }
    return entries;
}

QVariantList getPdfFiles(const QUrl folder) {

    QDir dir{folder.path()};
    const auto& entryList = dir.entryList(QDir::Files);

    qWarning() << dir;

    QVariantList pdfFiles;
    pdfFiles.reserve(entryList.size());

    for (const QString &filename : entryList) {

        if(!filename.endsWith(".pdf") && !filename.endsWith(".PDF")) {
            continue;
        }

        qWarning() << filename;

        pdfFiles.push_back(filename);
    }
    return pdfFiles;
}

}

Backend::Backend(QObject *parent)
    : QObject{parent},
      m_pdfFiles(getPdfFiles(m_folder))
{
}

QUrl Backend::folder() const
{
    return m_folder;
}

bool Backend::buttonStartEnabled() const
{
    return true;
    //return !m_pdfFiles.isEmpty();
}


QVariantList  Backend::pdfFiles() const
{
    return m_pdfFiles;
}

void Backend::setFolder(const QUrl &folder)
{
    if (m_folder == folder) {
        return;
    }
    m_folder = folder;

    qWarning() << folder;
    const auto pdfFiles = getPdfFiles(m_folder);
    _setPdfFiles(pdfFiles);
}

void Backend::tryConvertToCSV()
{
    QString program = "/home/sandro/Documents/bankconv/bankconv/pdf_to_utf8/dist/pdf_to_utf8";
    //QStringList arguments = {m_folder.path() + "/" + m_pdfFiles[0].toString()};

    qWarning() << "program exists:" << QFileInfo(program).exists();

    QStringList arguments = {"/mnt/Buisness/Dokumente/Beide/Haushalt/Kontoauszüge/Sparkasse Emsland/Girokonto Sandro/2015/2015.04 Sparkasse Emsland Kontoauszug 102303070.PDF"};

    QProcess process;
    process.start(program, arguments);
    if(!process.waitForStarted()) {
        qWarning() << "Could not start converter";
    }

    if (!process.waitForFinished()) {
        qWarning() << "Could not finnish conversion";
    }

    //QByteArray result = process.readAll();
    //QStringList rows = QString::fromUtf8(result).split("\n");

    QFile file("output.txt");
    if (file.open(QFile::WriteOnly | QFile::Truncate)) {
        QTextStream out(&file);

        const auto entries = toEntriesCheckingAccount(process.readAll());

        for(const auto& entry : entries) {
            out << entry.rawEntry << "\n";
        }
    }
}

void Backend::_setPdfFiles(const QVariantList& pdfFiles)
{
    if(m_pdfFiles == pdfFiles) {
        return;
    }
    m_pdfFiles = pdfFiles;
    emit pdfFilesChanged();

    }

} // namespace sps


/*

// find Kontostand and Save Value at the end
Kontostand am 04.03.2015, Auszug Nr.    3            6.953,30+

// Save Dates Buchung Valuta
06.03.2015 06.03.2015 Lastschrift
// Save Auftraggeber/Empf�nger / delete excess soaces
ARAL DOETLING A1            050307387187295171221200700
// until value copy all without space into Verwendungszweck
OLV71017761 05.03 07.38 ME7
// Copy Betrag without space
           52,91-
// Add to start value

10.03.2015 10.03.2015 Lastschrift
AMAZON EU S.A.R.L.                  5 RUE PLAETIS

302-3538322-9354736 Amazon.de 24234 81875217818

2423481875217818                    XAYE5JvujTtZLKXla23hhxeWgBF,Rz

Gläubiger-ID: DE24ZZZ00000561652

            9,93-
// marks the end
Kontostand am 31.03.2015 um 20:04 Uhr           10.834,02+

*/
