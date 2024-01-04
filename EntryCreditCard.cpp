#include "EntryCreditCard.hpp"

#include <QtCore/QTextStream>

namespace bankconv {

QTextStream& operator<<(QTextStream& os, const EntryCreditCard& obj)
{
    constexpr char separator = ';';

    os << obj.creditCardNumber   << separator<<
        obj.dateFirst << separator<<
        obj.dateSecond << separator<<
        obj.amount << separator<<  // TODO maybe we need to list conversion here instead
        obj.currency << separator<< // TODO maybe we need to list original currency here instead
        "1.00" << separator<<       // TODO maybe we need to list conversion factor
        obj.amount << separator<<
        obj.currency << separator<<
        obj.payee<< separator<< // Transaktionsbeschreibung = payee
        obj.bookingReason;  //"Transaktionsbeschreibung Zusatz"

    for(int i=0; i<6; ++i) {
        os << separator;
    }

        // skip remaining because an empty ; would confuse excel

//       << separator<<
//        "" << separator<<  // ??? unclear were we get the number from
//        "" << separator<<
//        "" << separator<<
//        "" << separator<<
//        "" << separator<<
//        "";  // ??? unclear were we get the number from

    return os;
}

}
