#include "EntryCheckingAccount.hpp"

#include <QtCore/QTextStream>

namespace bankconv {

QTextStream& operator<<(QTextStream& os, const EntryCheckingAccount& obj)
{
    constexpr char separator = ';';

    os << obj.accountNumber << separator << obj.dateFirst
               << separator << obj.dateSecond << separator << obj.payee
               << separator << obj.bookingType << separator
               << obj.bookingReason << separator << obj.saldo << separator
               << obj.currency << separator << obj.amount << separator
               << obj.currency;
    return os;
}

}
