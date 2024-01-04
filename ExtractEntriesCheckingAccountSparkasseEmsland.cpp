#include "ExtractEntriesCheckingAccountSparkasseEmsland.hpp"

#include "EntryCheckingAccount.hpp"
#include "ExtractUtils.hpp"

#include <QtCore/QRegularExpression>
#include <QtCore/QDate>
#include <QtCore/QDebug>
#include <QtCore/QTextStream>

#include <vector>
#include <set>

namespace bankconv {

namespace {

std::optional<QString> extractAccountNumber(const std::vector<QString>& rows)
{
    const auto maybeRow = findRowWhoStartsWith(rows, {
                                                         "Onlinekonto",
                                                         "Servicekonto",
                                                         "Emsland-Konto",
                                                         "EL-Konto PrivatStandard",
                                                         "Jugendgirokonto",
                                                         "Geldmarktkonto"
                                                     });
    if(!maybeRow) {
        return {};
    }

    const auto& row = *maybeRow;
    const auto items = row.split(',');
    if(items.size() != 2) {
        return {};
    }
    const auto accountNumber = items.back();
    return accountNumber.simplified();
}

std::optional<QString> extractCurrency(const std::vector<QString>& rows)
{
    const auto maybeRow = findRowWhoStartsWith(rows, {
                                                      "Datum Wert Erläuterung ",
                                                      "Datum Erläuterung Betrag "
    });
    if(!maybeRow) {
        return {};
    }
    const auto& row = *maybeRow;
    const auto items = row.split(' ');

    return items.back();
}

std::optional<QString> extractStartValue(std::vector<QString>::const_iterator &cit,
                                         const std::vector<QString> &rows)
{
    cit = std::find_if(cit, rows.cend(), [](const QString &row) {
        return row.startsWith(" Kontostand")
               || row.startsWith("Kontostand");
    });

    if(cit == rows.cend()) {
        return{};
    }

    auto startValue = extractAmount(*cit);
    return startValue;
}

bool isEndOfPageEntries(const QString &row)
{
    if(row.startsWith("Seite")) {
        return true;
    }
    if(row.startsWith("1.")) {
        return true;
    }
    return row == " ";
}

bool isEndOfDocument(const QString &row)
{
    return row.startsWith(" Kontostand am") || row.startsWith("Kontostand am");
}

QString extractDateFirst(const QString &row)
{
    // TODO error handling
    return row.section(' ', 0, 0);
}

QString extractDateSecond(const QString &row)
{
    // TODO error handling
    return row.section(' ', 1, 1);
}

bool entriesHaveTwoDates(const QString &row)
{
    const auto dateSecond = extractDateSecond(row);
    auto date = QDate::fromString(dateSecond, "dd.MM.yyyy");
    return date.isValid();
}

std::optional<QString> extractBookingType(bool hasTwoDates, const QString &row)
{
    const auto sectionStart = hasTwoDates ? 2 : 1;

           // TODO error handling
    const auto bookingType = row.section(' ', sectionStart);

           // Glue code to prevent that amount is extracted too
    constexpr const char* cashPayout = "Barauszahlung";
    if(bookingType.startsWith(cashPayout)) {
        return cashPayout;
    }
    constexpr const char* invoice = "Abrechnung";
    if(bookingType.startsWith(invoice)) {
        return invoice;
    }

    std::set<QString> validBookingTypes{
        "Lastschrift", "Geldautomat", "Dauerauftrag", "Überw.gutschrift",
        "Dauerauftrag", "Entgeltabrechnung", "Abrechnung", "Bareinzahlung",
        "Überw. beleglos", "Belastung", "entgeltfreie Buchung", "LS-Einzug beleglos",
        "Kartenzahlung", "Scheckgutschrift", "Laden Prepaid", "Gutschrift",
        "Kartenzahlungen", "LS-Einzug belegl."

    };

    if(validBookingTypes.count(bookingType) == 1) {
        return bookingType;
    };

    qWarning() << "invalid bookingtype: " << bookingType;

    return {};
}

bool isAmountWithSignAtTheEnd(const QString &amount)
{
    static QRegularExpression re{R"(^\d+,\d{2}(\+?|-)$)"};
    if(re.match(amount).hasMatch()) {
        return true;
    }
    return false;
}

bool isAmountWithSignInFront(const QString &amount)
{
    static QRegularExpression re{R"(^(-|\+?)\d+,\d{2}$)"};
    if(re.match(amount).hasMatch()) {
        return true;
    }
    return false;
}

bool isAmount(QString row)
{
    row = row.simplified();
    if(row.isEmpty()) {
        return false;
    }
    auto entries = row.split(' ');
    auto amount = entries.back();
    amount = amount.remove('.');
    if(isAmountWithSignAtTheEnd(amount)) {
        return true;
    }
    if(isAmountWithSignInFront(amount)) {
        return true;
    }
    return false;
}

bool hasAmountAtTheEnd(const QString& row)
{
    static QRegularExpression re{R"(\s{2,})"};
    auto entries = row.split(re);

    return isAmount(entries.back());
}

QString extractBookingReasonFromSingleRow(bool hasTwoDates,  QString row)
{
    row = row.simplified();
    const auto sectionStart = hasTwoDates ? 2 : 1;
    return row.section(' ', sectionStart, -1);
}

QString
extractBookingReason(std::vector<QString>::const_iterator &cit)
{
    QString bookingReason;
    while (!isAmount(*cit)) {
        if (!bookingReason.isEmpty()) {
            bookingReason += " ";
        }
        bookingReason += *cit;
        ++cit;
    }
    bookingReason = bookingReason.simplified();

    return bookingReason;
}

std::pair<QString, QString>
extractPayeeAndBookingReasonFirstRow(std::vector<QString>::const_iterator &cit, const QString& firstRow)
{
    QString payee;
    QString bookingReason;
    // normal case payee and bookingReason are separated by
    static QRegularExpression re{R"(\s{2,})"};
    auto entries = firstRow.split(re);

    qWarning() << entries;

           // hacky solution for rare case. We just split and take the first word
           // as the payee.
    if(entries.size() == 1){
        if(cit->startsWith("ARAL")) {
            entries = cit->split(' ');
            qWarning() << entries;
        }
    }

    payee = entries[0];

    for(auto citEntries = entries.cbegin() +1; citEntries != entries.cend(); ++citEntries) {
        if(!bookingReason.isEmpty()) {
            bookingReason += " ";
        }
        bookingReason += *citEntries;
    }

    ++cit;
    while (!isAmount(*cit)) {
        if (!bookingReason.isEmpty()) {
            bookingReason += " ";
        }
        bookingReason += *cit;
        ++cit;
    }
    bookingReason = bookingReason.simplified();

    return {payee, bookingReason};
}

std::pair<QString, QString>
extractPayeeAndBookingReason(std::vector<QString>::const_iterator &cit)
{
    return extractPayeeAndBookingReasonFirstRow(cit, *cit);
}

std::pair<QString, QString>
extractPayeeAndBookingReasonWithDateRow(bool hasTwoDates, std::vector<QString>::const_iterator &cit)
{
    const auto sectionStart = hasTwoDates ? 2 : 1;

           // row without [date date]
    auto row = cit->section(' ', sectionStart);
    return extractPayeeAndBookingReasonFirstRow(cit, row);
}

std::pair<QString, QString>
extractPayeeAndBookingReason(std::vector<QString>::const_iterator &cit,
                             const QString& bookingType)
{
    if(bookingType == "Entgeltabrechnung" || bookingType == "Abrechnung") {
        ++cit;
        // TODO maybe not hardcode institute here.
        return { "Sparkasse Emsland", ""};
    }
    if(bookingType == "Geldautomat" || bookingType == "Bareinzahlung") {
        // Not payee known for Geldautomat
        return { QString{}, extractBookingReason(cit)};
    }
    return extractPayeeAndBookingReason(cit);

}

std::optional<EntryCheckingAccount> extractEntry(
    bool hasTwoDates,
    std::vector<QString>::const_iterator &cit,
    const QString& saldoLast,
    const QString& accountNumber,
    const QString& currency)
{

    const auto dateFirst = extractDateFirst(*cit);
    qWarning() << "dateFirst: " << dateFirst;
    const auto dateSecond = hasTwoDates ? extractDateSecond(*cit) : dateFirst;
    qWarning() << "dateSecond: " << dateSecond;
    const auto maybeBookingType = extractBookingType(hasTwoDates, *cit);

    QString bookingType;
    QString payee;
    QString bookingReason;

    if(!maybeBookingType) {
        if(hasAmountAtTheEnd(*cit)) {
            bookingReason = extractBookingReasonFromSingleRow(hasTwoDates, *cit);
        }
        else {
            const auto [payeeInternal, bookingReasonInternal] =
                extractPayeeAndBookingReasonWithDateRow(hasTwoDates, cit);
            payee = payeeInternal;
            bookingReason = bookingReasonInternal;
        }
    }
    else {
        bookingType = *maybeBookingType;
        qWarning() << "bookingType: " << bookingType;

        if(bookingType == "Barauszahlung") {
            // Do nothing we can only get amount here
        }
        else {
            ++cit;
            const auto [payeeInternal, bookingReasonInternal] =
                extractPayeeAndBookingReason(cit, bookingType);
            payee = payeeInternal;
            bookingReason = bookingReasonInternal;
        }
    }
    qWarning() << "payee: " << payee;
    qWarning() << "bookingReason: " << bookingReason;

    const auto amount = extractAmount(*cit);
    qWarning() << "amount:" << amount;

    const auto saldo = calculateSaldo(saldoLast, amount);
    qWarning() << "saldo: " << saldo;

    return EntryCheckingAccount{
                                accountNumber,
                                dateFirst,
                                dateSecond,
                                payee,
                                bookingType,
                                bookingReason,
                                saldo,
                                currency,
                                amount};
}

void tryGoToNextPageEntries(std::vector<QString>::const_iterator &cit,
                            const std::vector<QString>& rows)
{
    cit = std::find_if(cit, rows.cend(), [](const QString &row) {
        return row.startsWith("Datum Wert Erläuterung") ||
               row.startsWith("Datum Erläuterung Betrag");
    });
    ++cit;
}

}


std::vector<EntryCheckingAccount>
extractEntriesCheckingAccountSparkasseEmsland(const std::vector<QString>& rows)
{
    const auto maybeAccountNumber = extractAccountNumber(rows);
    if(!maybeAccountNumber) {
        qWarning() << "Error: No Account Number";
        return{};
    }
    const auto& accountNumber = *maybeAccountNumber;

    const auto maybeCurrency = extractCurrency(rows);
    if(!maybeCurrency) {
        qWarning() << "Error: No Currency";
        return{};
    }
    const auto& currency = *maybeCurrency;

    auto cit = rows.begin();
    const auto maybeStartValue = extractStartValue(cit, rows);
    if(!maybeStartValue) {
        qWarning() << "No start value";
        return {};
    }
    const auto& startValue = *maybeStartValue;
    qWarning() << "startValue: " << startValue;

    ++cit;

           // At some point one date was omitted so we need to handle this here.
           // not optimal we could check this only once.
    const auto hasTwoDates = entriesHaveTwoDates(*cit);

    auto saldoLast = startValue;

    std::vector<EntryCheckingAccount> entries{};
    for (;;) {
        // Run until end of document
        for(;;) {
            // Run until current page is finnished
            if(isEndOfPageEntries(*cit)) {
                break;
            }
            if(isEndOfDocument(*cit)) {
                // Sanity check. Calculated saldo should be same as the one in
                // the file
                qWarning() << "*cit: " << *cit;
                qWarning() << "extractAmount(*cit): " << extractAmount(*cit);
                qWarning() << "entries.back().saldo): " << entries.back().saldo;
                Q_ASSERT(extractAmount(*cit) == entries.back().saldo);
                return entries;
            }
            if(const auto& maybeEntry =
                extractEntry(hasTwoDates, cit, saldoLast, accountNumber, currency);
                maybeEntry.has_value()) {
                entries.emplace_back(*maybeEntry);
                saldoLast = entries.back().saldo;
            }
            ++cit;
            Q_ASSERT(cit != rows.end());
        }
        tryGoToNextPageEntries(cit, rows);
    }
    return {};
}

}

