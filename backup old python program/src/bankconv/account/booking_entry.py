# account number  -> Auftragskonto
# booking date -> Buchungstag
# recite date -> Valutadatum
# type -> Buchungstext
#
# -> Lastschrift: payee + description + optional payee_id
#
# -> Geldautomat: description
#
# -> Dauerauftrag: payee + description
#
# -> Überw.gutschrift: payee + description
#
# -> Überw.beleglos: payee + description
#
# -> Bareinzahlung: description
#
# -> Entgeltabrechnung:  nothing (Add Sparkasse here)
#
#
#
#
#
#
#
# description -> Verwendungszweck
# payee_id -> Gläubiger Id
# payee -> Beguenstigter/Zahlungspflichtiger
# bank account payee -> Kontonummer/IBAN
# currency -> Waehrung
# amount -> Betrag

from typing import List


class BookingEntry:
    def __init__(
        self, account_number: str, currency: str, data_block: List[str]
    ):
        self.account_number = account_number
        self.currency = currency

        [
            self.booking_date,
            self.recite_date,
            self.type,
            self.description,
            self.payee,
            self.payee_account,
            self.payee_id,
            self.amount,
        ] = self._get_from_data_block(data_block)

    def _get_from_data_block(self, data_block: List[str]) -> List[str]:
        """
        Parses datablock for
        booking_date: str
        recite_date: str
        type: str
        description: str
        payee: str
        payee_account: str
        payee_id: str
        amount: str
        """
        return ["", "", "", "", "", "", "", "", ""]

    def __repr__(self) -> str:
        return self._get_class_presentation()

    def __str__(self) -> str:
        return self._get_class_presentation()

    def _get_class_presentation(self) -> str:
        """
        Helper to show member of class for __str__ and __repr__
        """
        print_str: str = ""
        print_str += "{} ".format(self.account_number)
        print_str += "{} ".format(self.booking_date)
        print_str += "{}".format(self.recite_date)
        print_str += "{}".format(self.type)
        print_str += "{} ".format(self.description)
        print_str += "{} ".format(self.currency)
        print_str += "{} ".format(self.amount)
        print_str += "{} ".format(self.payee)
        print_str += "{} ".format(self.payee_account)
        print_str += "{} ".format(self.payee_id)
        return print_str
