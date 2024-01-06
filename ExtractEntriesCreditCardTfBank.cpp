#include "ExtractEntriesCreditCardSparkasseEmsland.hpp"

#include "EntryCreditCard.hpp"
#include "ExtractUtils.hpp"

#include <QtCore/QList>
#include <QtCore/QDebug>
#include <QtCore/QDate>

namespace bankconv {

namespace {

std::optional<QString> extractCreditCardNumber(QString row)
{
    if(row.isEmpty()) {
        return {};
    }

    const auto entries = row.split(" ");
    // shitty way to detect credit card in format 123456******7890
    auto creditCardNumber = entries[0];
    if(!creditCardNumber.contains("******")) {
        return {};
    }
    if(creditCardNumber.size() != 16)  {
        return {};
    }

    // fix format to 1234 56** **** 7890
    creditCardNumber.insert(12, ' ');
    creditCardNumber.insert(8, ' ');
    creditCardNumber.insert(4, ' ');

    return creditCardNumber;
}

std::optional<QString> extractCreditCardNumber(const std::vector<QString>& rows)
{
    // find row 2 rows away from relevant row
    auto it = std::ranges::find_if(rows,[](const QString& row){
        if(row.startsWith("Kartennummer")) {
            return true;
        }
        return false;
    });

    if(it == rows.end()) {
        qWarning() << "Could not extract credit card number ";
        return {};
    }

    // go to row which contains credit card number
    it += 2;
    if(it == rows.end()) {
        qWarning() << "Could not extract credit card number ";
        return {};
    }

    const auto maybeCreditCardNumber = extractCreditCardNumber(*it);
    if(!maybeCreditCardNumber) {
        qWarning() << "Could not extract credit card number ";
        return {};
    }
    return *maybeCreditCardNumber;
}

std::optional<QString> extractCurrency(QString row)
{
    row = row.simplified();

    const auto entries = row.split(" ");
    if(entries.size() != 2) {
        qWarning() << "Could not extract currency ";
        return {};
    }
    return entries.back();
}

std::optional<QString> extractCurrency(const std::vector<QString>& rows)
{
    const auto maybeRowCurrency = findRowWhoStartsWith(rows, {"Währung:"});
    if(!maybeRowCurrency) {
        qWarning() << "Could not extract row date ";
        return {};
    }
    return extractCurrency(*maybeRowCurrency);
}

std::vector<QString>::const_iterator goToStartOfRelevantData(
    const std::vector<QString> &rows)
{
    return std::find(rows.cbegin(), rows.cend(), "KR");
}

std::optional<QString> extractDateFirst(const QString &row)
{
    Q_ASSERT(!row.simplified().isEmpty());

    const auto& entries = row.split(" ");
    Q_ASSERT(entries.size() >= 2);
    // date first is after second date in the line.
    const auto dateRaw = entries[2];
    qWarning() << "dateRaw: " << dateRaw;
    const auto date = QDate::fromString(dateRaw, "dd.MM.yyyy");
    if(!date.isValid()) {
        // valid case. In some cases only one date exists
        return {};
    }
    return dateRaw;
}

QString extractDateSecond(const QString &row)
{
    Q_ASSERT(!row.simplified().isEmpty());

    const auto& entries = row.split(" ");
    Q_ASSERT(entries.size() >= 2);
    // date first is after second date in the line.
    const auto dateRaw = entries[1];
    const auto date = QDate::fromString(dateRaw, "dd.MM.yyyy");
    Q_ASSERT(date.isValid());
    if(!date.isValid()) {
        return {};
    }
    return dateRaw;
}

QString extractPayee(const QString& row)
{
    const auto entries = row.split(" ");

    auto cit = std::find(entries.cbegin(), entries.cend(), "Kauf");
    if(cit == entries.cend()) {

        cit = std::find(entries.cbegin(), entries.cend(), "Bezahlung");
        if(cit == entries.cend()) {

            cit = std::find(entries.cbegin(), entries.cend(), "Bankautomat");
            if(cit == entries.cend()) {
                cit = std::find(entries.cbegin(), entries.cend(), "Zinsen");
                if(cit == entries.cend()) {
                    Q_ASSERT(false);
                    return {};
                }
                // hack to include Zinsen in payee. Refactor this if necessary.
                --cit;
                --cit;
            }
        }
    }
    ++cit;
    ++cit;
    QString payee;
    for(;;) {
        // end of payee
        if(*cit == "Wechselkurs") {
            break;
        }
        // potential dirty hack to detect something like Kredit -3.42 D"
        // consider to search for whole amount here.
        if(cit->startsWith("-")) {
            break;
        }
        if(cit == entries.cend()) {
            return{};
        }
        if(!payee.isEmpty()) {
            payee += " ";
        }
        payee += *cit;
        ++cit;
    }
    return payee;
}

QString extractBookingReason(const QString& row)
{
    const auto entries = row.split(" ");
    auto cit = std::find(entries.cbegin(), entries.cend(), "Wechselkurs");
    if(cit == entries.cend()) {
        return{};
    }
    const auto index = std::distance(entries.cbegin(), cit);

    Q_ASSERT(entries.size() > index + 1);
    if(entries[index + 1] == "1.00000") {
        return {}; // Unit is Euro so no conversion.
    }

    QString bookingReason;
    for(int i=index; i < entries.size() -2; ++i) {
        bookingReason += entries[i];
        if(i != entries.size() -3) {
            bookingReason += " ";
        }
    }
    return bookingReason;
}


QString extractAmountCreditCardTfBank(QString row)
{
    const auto entries = row.split(" ");

    Q_ASSERT(entries.size() > 2);
    auto amount = entries[entries.size() -2];
    amount.replace('.', ',');
    return amount;
}

std::optional<EntryCreditCard> extractEntry(
    std::vector<QString>::const_iterator &cit,
    const std::vector<QString> &rows,
    const QString& saldoLast,
    const QString& creditCardNumber,
    const QString& currency)
{
    const auto dateSecond = extractDateSecond(*cit);
    qWarning() << "dateSecond: " << dateSecond;

    const auto dateFirst = [dateSecond, cit]{
        const auto maybeDateFirst = extractDateFirst(*cit);
        if(maybeDateFirst) {
            return *maybeDateFirst;
        }
        return dateSecond;
    }();
    qWarning() << "dateFirst: " << dateFirst;

    QString row;
    for(;;) {
        if(!row.isEmpty()) {
            row += " ";
        }
        row += *cit;

        // currency is not present if we payed with annother currency
        if(row.contains(currency) || row.endsWith(" D")) {
            break;
        }

        ++cit;
        if(cit == rows.end()) {
            qWarning() << "cit == rows.end() ";
            return {};
        }
    }
    row = row.simplified();
    // Fore some reason data from rows does not have the A
    row = row.replace("�","A");
    qWarning() << "row: " << row;

    /*
    // payee bookingReason amount
    710515221 27.11.2023 24.11.2023 Kauf 56007040 REWE Mering Wallbergst Wallbergstraße 16 Wechselkurs 1.00000 86.30 EUR -86.30 D
    */

    const auto payee = extractPayee(row);
    qWarning() << "payee: " << payee;

    const auto bookingReason = extractBookingReason(row);
    qWarning() << "booking reason" << bookingReason;

    const auto amount = extractAmountCreditCardTfBank(row);
    qWarning() << "amount: " << amount;

    const auto saldo = [saldoLast, amount]{
        if(!amount.startsWith("-")) {
            return saldoLast;
        }
        return calculateSaldo(saldoLast, amount);
    }();
    qWarning() << "saldo: " << saldo;

    return EntryCreditCard{
        creditCardNumber,
        dateFirst,
        dateSecond,
        payee,
        bookingReason,
        saldo,
        currency,
        amount
    };
}

bool isEndOfDocument(const QString &row)
{
    return row.startsWith("Verwendeter Gesamtbetrag:");
}

bool isEndOfPageEntries(const QString &row)
{
    return row == R"(\xa0 \xa0)";
}

void tryGoToNextPageEntries(std::vector<QString>::const_iterator &cit,
                            const std::vector<QString>& rows)
{
    cit = std::find(cit, rows.cend(), "Schweden");
    ++cit;  // empty space
    ++cit;  // next row
    Q_ASSERT(cit != rows.cend());
}

}

/*

KR   // first page data
[entry0]
[entry1]
\xa0 \xa0  // end of page

Schweden cit += 2 // Start of second page
[entry2]
[entry3]
Verwendeter Gesamtbetrag:  // End of all

 */


std::vector<EntryCreditCard> extractEntriesCreditCardTfBank(const std::vector<QString> &rows)
{
    const auto maybeCreditCardNumber = extractCreditCardNumber(rows);
    if(!maybeCreditCardNumber) {
        qWarning() << "Error: No Credit Card Number";
        return{};
    }
    const auto& creditCardNumber = *maybeCreditCardNumber;
    qWarning() << "creditCardNumber: " << creditCardNumber;

    const auto maybeCurrency = extractCurrency(rows);
    if(!maybeCurrency) {
        qWarning() << "Error: No Currency";
        return{};
    }
    const auto& currency = *maybeCurrency;
    qWarning() << "currency: " << currency;

    // start value should be 0 for now we assume credit card
    const auto startValue = QString{"0"};
    qWarning() << "startValue: " << startValue;

    auto saldoLast = startValue;

    auto cit = goToStartOfRelevantData(rows);
    if(cit == rows.cend()) {
        qWarning() << "Could not find start value";
        return{};
    }

    ++cit;

    std::vector<EntryCreditCard> entries{};
    std::optional<QString> amountCompensationPreviousPage;

    for(;;) {
        qWarning() << "*cit before:" <<*cit;
        if(const auto& maybeEntry =
            extractEntry(
                cit,
                rows,
                saldoLast,
                creditCardNumber,
                currency);
            maybeEntry.has_value()) {
            entries.emplace_back(*maybeEntry);
            saldoLast = entries.back().saldo;
        }
        else {
            Q_ASSERT(false);
        }
        ++cit;
        qWarning() << "*cit after2: " <<*cit;

        if(isEndOfDocument(*cit)) {
            saldoLast = entries.back().saldo;
            // Sanity check. Calculated saldo should be same as the one in
            // the file

            {
                auto amount = *cit;
                amount = amount.remove("Verwendeter Gesamtbetrag:");
                // remove stupid spaces like -2 386.79
                amount = amount.remove(' ');
                amount.replace('.', ',');

                qWarning() << "amount: " << amount;
                qWarning() << "saldoLast: " << saldoLast;
                Q_ASSERT(amount == saldoLast);
            }
            return entries;
        }
        if(isEndOfPageEntries(*cit)) {
            tryGoToNextPageEntries(cit, rows);
            Q_ASSERT(cit != rows.cend());
        }
    }

    Q_ASSERT(false);
    return {};
}

}
