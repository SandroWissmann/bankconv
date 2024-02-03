#include "EntryCreditCard.hpp"

#include <QtCore/QTextStream>

namespace bankconv {

namespace {

}

QTextStream& operator<<(QTextStream& os, const EntryCreditCard& obj)
{
    constexpr char separator = ';';

    os << obj.creditCardNumber   << separator;
    os << obj.dateFirst << separator;
    os << obj.dateSecond << separator;
    os << obj.amount << separator;  // TODO maybe we need to list conversion here instead
    os << obj.currency << separator; // TODO maybe we need to list original currency here instead
    os << "1.00" << separator;       // TODO maybe we need to list conversion factor
    os << obj.amount << separator;
    os << obj.currency << separator;

    if(!obj.payee.isEmpty()) {
        os << obj.payee<< separator; // Transaktionsbeschreibung = payee
    }

    if(!obj.bookingReason.isEmpty()) {
        os << obj.bookingReason << separator;  //"Transaktionsbeschreibung Zusatz"
    }
    os << separator; //Buchungsreferenz;
    os << separator;//Geb�hrenschl�ssel;
    os << separator;//L�nderkennzeichen;
    os << separator;//BAR-Entgelt+Buchungsreferenz;
    os << separator;//AEE+Buchungsreferenz;
    os << separator;//Abrechnungskennzeichen

//    for(int i=0; i<5; ++i) {
//        os << separator;
//    }

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
