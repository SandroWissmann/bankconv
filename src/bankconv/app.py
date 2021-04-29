#!/usr/bin/env python3

from typing import List
from typing import Union

from dataclasses import dataclass

import tkinter as tk
import tkinter.filedialog

import os

from tika import parser


@dataclass
class CreditCardEntry:
    credit_card_number: str
    recite_day: int
    recite_month: int
    recite_year: int
    booking_day: int
    booking_month: int
    booking_year: int
    recite_date: str
    booking_date: str
    amount: float
    currency: str
    description: str


class CreditCardBilling:
    def __init__(self):
        None

    def add_billing_data(self, text_lines: List[str]):
        print("add billing to year")


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

    credit_card_billings: List[CreditCardBilling] = []

    filenames_pdf: List[str] = os.listdir(directory)
    filenames_pdf.sort()
    for filename_pdf in filenames_pdf:
        if not filename_pdf.lower().endswith(".pdf"):
            continue

        text_lines: List[str] = get_text_lines_from_pdf(
            directory, filename_pdf
        )

        filename_txt_absolut: str = get_filename_txt_absolut(
            directory, filename_pdf
        )

        write_text_lines_to_file_in_dir(filename_txt_absolut, text_lines)

    # create attach each pdf to an excel file going by year


if __name__ == "__main__":
    main()
