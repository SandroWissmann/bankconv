#ifndef BANKCONV_ENTRYCHECKINGACCOUNT_HPP
#define BANKCONV_ENTRYCHECKINGACCOUNT_HPP

#include <QtCore/QString>

#include <vector>

class QTextStream;

namespace bankconv {

struct EntryCheckingAccount {
    QString accountNumber; // Auftragskonto
    QString dateFirst;     // Buchung -> first date
    QString dateSecond;    // Valuata -> second date
    QString payee;         // Auftragsgeber/Empfänger
    QString bookingType;   // Buchungstext
    QString bookingReason; // Verwendungszweck
    QString saldo;         // Saldo -> How much is on the account after booking
    QString currency;      // WÄhrung
    QString amount;        // Betrag
};

std::vector<EntryCheckingAccount> toEntriesCheckingAccount(const std::vector<QString>& rows);

QTextStream& operator<<(QTextStream& os, const EntryCheckingAccount& obj);

}


#endif
