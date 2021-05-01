#!/usr/bin/env python3

from typing import List
from typing import Union
from typing import Optional

import re

import tkinter as tk
import tkinter.filedialog

import os

from tika import parser


class CreditCardEntry:
    def __init__(
        self,
        credit_card_number: str,
        currency: str,
        line: str,
        start_year: str = None,  # None or str
        end_date: str = None,  # None or str
    ):
        self.credit_card_number = credit_card_number
        self.currency = currency
        if end_date is None:
            booking_date, recite_date = self._get_booking_and_recite_date(line)
            self.booking_date = booking_date + start_year
            self.recite_date = recite_date + start_year
        else:
            self.booking_date = end_date[:-4] + end_date[-2:]
            self.recite_date = self.booking_date
        self.description = self._get_description(line)
        self.description_addition = ""
        self.amount = self._get_amount(line)

    def _get_booking_and_recite_date(self, line: str) -> List[str]:
        """
        Searches line for booking and recite date.
        Return booking and recite date in format DD.MM. as List.
        Assumes that line contains valid booking and recite date.
        """
        match = re.match(r"\d{2}.\d{2}. \d{2}.\d{2}.", line)
        assert match
        dates = match.group(0).split()
        assert len(dates) == 2, "booking or recite date missing"
        return [dates[0], dates[1]]

    def _get_description(self, line: str) -> str:
        """
        Searches line for description and returns it.
        """
        date_match = re.match(r"\d{2}.\d{2}. \d{2}.\d{2}.", line)
        if date_match:
            line = line[date_match.end() :]
        line = line.lstrip()

        match = re.search(r"\d{1,3}(.\d{3})*,\d{2}(\+|-){1}", line)
        line = line[: match.start()]
        return line.rstrip()

    def _get_amount(self, line: str) -> str:
        """
        Searches line for amount in the format
        D+,DD(+/-)
        and returns it as
        (-?)D+.DD
        """
        match = re.search(r"\d{1,3}(.\d{3})*,\d{2}(\+|-){1}", line)
        assert match, "line: {}".format(line)
        amount = match.group(0)
        last_char = amount[-1]
        if amount[-1] == "-":
            return "-" + amount[:-1]
        return amount[:-1]

    def __repr__(self) -> str:
        return self._get_class_presentation()

    def __str__(self) -> str:
        return self._get_class_presentation()

    def _get_class_presentation(self) -> str:
        """
        Helper to show member of class for __str__ and __repr__
        """
        print_str: str = ""
        print_str += "{} ".format(self.credit_card_number)
        print_str += "{} ".format(self.booking_date)
        print_str += "{} ".format(self.recite_date)
        print_str += "{} ".format(self.currency)
        print_str += "{}\t".format(self.amount)
        print_str += "{} ".format(self.description)
        if self.description_addition != "":
            print_str += "{} ".format(self.description_addition)
        return print_str


class CreditCardBilling:
    def __init__(self):
        self.credit_card_entries: List[CreditCardEntry] = []

    def add_monthly_billing_data(self, text_lines: List[str]):
        self._extract_data_from_text_lines(text_lines)
        print("add billing to year")

    def _extract_data_from_text_lines(
        self, text_lines: List[str]
    ) -> Union[None, List[CreditCardEntry]]:
        None

        # find Abrechnung / Saldenmitteilung bis zum
        # Extract date
        index, end_date = self._get_end_date(text_lines)
        if type(end_date) is not str:
            print("End date not found in file")
            return

        print("{}\t{}".format(index, end_date))

        # iterate over text_lines until MasterCard row is found
        # extract credit card number
        index, credit_card_number = self._get_credit_card_number(
            text_lines, index + 1
        )
        if type(credit_card_number) is not str:
            print("Credit card number not found in file")
            return

        print("{}\t{}".format(index, credit_card_number))

        # go two more row to in EUR
        # extract currency = Eur
        index, currency = self._get_currency(text_lines, index + 1)
        if type(currency) is not str:
            print("Currency not found in file")
            return

        print("{}\t{}".format(index, currency))

        # Find Saldovortrag two more rows away.
        # extract date
        index, start_date = self._get_start_date(text_lines, index + 1)
        if type(start_date) is not str:
            print("Start date not found in file")
            return

        print("{}\t{}".format(index, start_date))

        start_year = self._get_year_from_date(start_date)

        print(start_year)

        index, credit_card_entries = self._get_credit_card_entries(
            text_lines, index + 1, credit_card_number, currency, start_year
        )
        if type(credit_card_entries) is not List[CreditCardEntry]:
            print("No bookings for the Month found")

        print(index)
        for credit_card_entry in credit_card_entries:
            print(credit_card_entry)

        credit_card_compensation = self._get_credit_card_compensation(
            text_lines, index + 1, credit_card_number, currency, end_date
        )
        if type(credit_card_compensation) is not CreditCardEntry:
            print("No compensation found")
            return

        print(credit_card_compensation)

        # while not Einzug von Kto.

        # Find first row without only spaces
        # extract booking date
        # extract recite date
        # extract description
        # extract amount

        # while next rows not contain only spaces
        # attach row content to description

        # Make Einzug von KTO
        # description booking date == recite date == saldendate

    def _get_end_date(
        self, text_lines: List[str]
    ) -> Optional[List[Union[int, str]]]:
        """
        Search for end date in text lines
        Returns line number and end date or none if not result
        """
        for line_number, text_line in enumerate(text_lines):
            if text_line.startswith("Abrechnung / Saldenmitteilung bis zum"):
                end_date = self._find_date(text_line)
                if type(end_date) is not str:
                    return None
                return [line_number, end_date]
        return None

    def _find_date(self, line: str) -> Optional[str]:
        """
        Search line for date in the format DD.MM.YYYY
        """
        match = re.search(r"\d{2}.\d{2}.\d{4}", line)
        if match:
            return match.group(0)
        return None

    def _get_credit_card_number(
        self, text_lines: List[str], start_line: int
    ) -> Optional[List[Union[int, str]]]:
        """
        Search for credit_card_number in text lines
        Returns line number and credit_card_number or None if not result
        """
        for line_number, text_line in enumerate(
            text_lines[start_line:], start_line
        ):
            if text_line.startswith("MasterCard"):
                credit_card_number = self._find_credit_card_number(text_line)
                if type(credit_card_number) is not str:
                    return None
                credit_card_number: str = self._reformat_credit_card_number(
                    credit_card_number
                )
                return [line_number, credit_card_number]

    def _find_credit_card_number(self, line: str) -> Optional[str]:
        """
        Search line for credit card number in the format
        DDDD DDXX XXXX DDDD.
        """
        match = re.search(r"\d{4} \d{2}XX XXXX \d{4}", line)
        if match:
            return match.group(0)
        return None

    def _reformat_credit_card_number(self, credit_card_number) -> str:
        """
        Reformats credit card number from Format
        DDDD DDXX XXXX DDDD to DDDD **** **** DDDD.
        Reason for this is that the automatic CSV export from Sparkasse
        is in DDDD **** **** DDDD and not DDDD DDXX XXXX DDDD.
        """
        parts: List[str] = credit_card_number.split()
        assert len(parts) == 4, "Credit card parts should be 4"
        return parts[0] + " **** **** " + parts[3]

    def _get_currency(
        self, text_lines: List[str], start_line: int
    ) -> Optional[List[Union[int, str]]]:
        """
        Search for currency in text lines
        Returns line number and currency or None if not result
        """
        for line_number, text_line in enumerate(
            text_lines[start_line:], start_line
        ):
            if text_line.find("EUR") != -1:
                return [line_number, "EUR"]
        return None

    def _get_start_date(
        self, text_lines: List[str], start_line: int
    ) -> Optional[List[Union[int, str]]]:
        """
        Search for start date in text lines
        Returns line number and start date or none if not result
        """
        for line_number, text_line in enumerate(
            text_lines[start_line:], start_line
        ):
            if text_line.find("Saldovortrag vom") != -1:
                start_date = self._find_date(text_line)
                if type(start_date) is not str:
                    return None
                return [line_number, start_date]
        return None

    def _get_year_from_date(self, date: str) -> str:
        """
        Expects date in format DD.MM.YYYY.
        Returns year in format YY.
        """
        return date[-2:]

    def _get_credit_card_entries(
        self,
        text_lines: List[str],
        start_line: int,
        credit_card_number: str,
        currency: str,
        start_year: str,
    ) -> List[Union[int, List[CreditCardEntry]]]:
        """
        Search for credit card entries in text lines
        Returns credit card entries and line_number
        """
        credit_card_entries: List[CreditCardEntry] = []

        # last line was currency
        found_entry: bool = False
        for line_number, text_line in enumerate(
            text_lines[start_line:], start_line
        ):
            if self._is_end_of_booking_line(text_line):
                break
            if text_line.isspace():
                found_entry = False
                continue
            if not self._is_credit_card_entry(text_line):
                if found_entry:
                    description_addition = text_line.strip()
                    if credit_card_entries[-1].description_addition != "":
                        credit_card_entries[-1].description_addition += "\t"
                    credit_card_entries[
                        -1
                    ].description_addition += description_addition
                else:
                    print("Unknown: {}".format(text_line))
                continue
            found_entry = True
            credit_card_entry = CreditCardEntry(
                credit_card_number, currency, text_line, start_year, None
            )
            credit_card_entries.append(credit_card_entry)

        return [line_number, credit_card_entries]

    def _get_credit_card_compensation(
        self,
        text_lines: List[str],
        start_line: int,
        credit_card_number: str,
        currency: str,
        end_date: str,
    ) -> Optional[CreditCardEntry]:
        """
        Searches text lines for special credit card entry after the normal
        entries which represents the compensation of the credit card.
        """
        for line_number, text_line in enumerate(
            text_lines[start_line:], start_line
        ):
            if text_line.find("Einzug von Kto.") == -1:
                continue
            return CreditCardEntry(
                credit_card_number, currency, text_line, None, end_date
            )
        return None

    def _is_end_of_booking_line(self, line: str) -> bool:
        """
        Returns true if end of booking line is found
        """
        return line.lstrip().startswith("--------------")

    def _is_credit_card_entry(self, line: str) -> bool:
        """
        Checks if passed line is a credit card enty in the format:
        DD.MM. DD.MM.   Description    Amount (+ or -)
        """
        match = re.match(
            r"\d{2}.\d{2}. \d{2}.\d{2}. {2}.+\d{1,3}(.\d{3})*,\d{2}(\+|-){1}",
            line,
        )
        return bool(match)


def get_directory() -> str:
    """
    Select directory from GUI User input and return it.
    """
    # get directory
    # root = tk.Tk()
    # root.withdraw()
    # directory = tk.filedialog.askdirectory()
    directory = "/mnt/Buisness/Haushalt/Kreditkartenabrechnungen Sandro"
    return directory


def get_text_lines_from_pdf(directory: str, filename_pdf: str) -> List[str]:
    """
    Extract Text from PDF file.
    Requires input to be a pdf file.
    Returns dict with lines of text in PDF file.
    """
    filename_pdf_absolut: str = os.path.join(directory, filename_pdf)

    raw: str = str(parser.from_file(filename_pdf_absolut))
    text: str = str(raw.encode("utf-8", errors="ignore"))

    text = text.replace(r"\xc3\xa4", "ä")
    text = text.replace(r"\xc3\xb6", "ö")
    text = text.replace(r"\xc3\xbc", "ü")
    text = text.replace(r"xc3x9c", "Ü")
    text = text.replace(r"\xc3\x9f", "ß")

    text_lines: List[str] = text.split("\\n")
    text_lines = [s.replace("\\", "") for s in text_lines]
    return text_lines


def get_filename_txt_absolut(directory: str, filename_pdf: str) -> str:
    """
    Get txt filename absolut based on pdf filename.
    """
    filename_txt: str = os.path.splitext(filename_pdf)[0] + ".txt"
    filename_txt_absolut: str = os.path.join(directory, filename_txt)
    return filename_txt_absolut


def write_text_lines_to_file_in_dir(
    filename_txt_absolut: str, text_lines: List[str]
) -> None:
    """
    Write text lines to file.
    """
    text_file: file = open(filename_txt_absolut, "w")
    for text_line in text_lines:
        text_file.write(text_line)
        text_file.write("\n")
    text_file.close()


def main():
    # iterate over all pdf files in folder
    directory: str = get_directory()

    credit_card_billing: CreditCardBilling = CreditCardBilling()

    filenames_pdf: List[str] = os.listdir(directory)
    filenames_pdf.sort()
    for filename_pdf in filenames_pdf:
        if not filename_pdf.lower().endswith(".pdf"):
            continue

        text_lines: List[str] = get_text_lines_from_pdf(
            directory, filename_pdf
        )

        credit_card_billing.add_monthly_billing_data(text_lines)

        filename_txt_absolut: str = get_filename_txt_absolut(
            directory, filename_pdf
        )

        write_text_lines_to_file_in_dir(filename_txt_absolut, text_lines)
        break

    # create attach each pdf to an excel file going by year


if __name__ == "__main__":
    main()
