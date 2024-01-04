#ifndef BANKCONV_ENTRYCREDITCARD_HPP
#define BANKCONV_ENTRYCREDITCARD_HPP

#include <QtCore/QString>

#include <vector>

class QTextStream;

namespace bankconv {

struct EntryCreditCard {
    QString creditCardNumber;
    QString dateFirst;
    QString dateSecond;
    QString payee;
    QString bookingReason;
    QString saldo; // Saldo -> How much is on the account after booking
    QString currency;
    QString amount;
};

QTextStream& operator<<(QTextStream& os, const EntryCreditCard& obj);

}


#endif
