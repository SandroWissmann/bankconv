#!/usr/bin/env python3

from typing import List
from typing import Union
from typing import Optional

from dataclasses import dataclass

import re

import tkinter as tk
import tkinter.filedialog

import os

from tika import parser


@dataclass
class CreditCardEntry:
    credit_card_number: str
    booking_day: int
    booking_month: int
    booking_year: int
    recite_day: int
    recite_month: int
    recite_year: int
    amount: float
    currency: str
    description: str


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
        index: int = 0
        index, booking_date = self._get_booking_date(text_lines, index)
        print(booking_date)
        print(index)
        if type(booking_date) is not str:
            print("No booking date found")
            return

        # iterate over text_lines until MasterCard row is found
        # extract credit card number

        # go two more row to in EUR
        # extract currency = Eur

        # Find Saldovortrag two more rows away.
        # extract date

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

    def _get_booking_date(
        self, text_lines: List[str], index: int
    ) -> Optional[List[Union[int, str]]]:
        for count, text_line in enumerate(text_lines):
            if text_line.startswith("Abrechnung / Saldenmitteilung bis zum"):
                match = re.search(r"\d{2}.\d{2}.\d{4}", text_line)
                if not match:
                    return None
                return [count, match.group(0)]
        return None


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
