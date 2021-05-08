#!/usr/bin/env python3

import os

from typing import List

import tkinter as tk
import tkinter.filedialog

import pdf_parser

from bankconv.credit_card_billing import CreditCardBilling


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


def main():
    directory: str = get_directory()

    credit_card_billing: CreditCardBilling = CreditCardBilling()

    filenames_pdf: List[str] = os.listdir(directory)
    filenames_pdf.sort()
    for filename_pdf in filenames_pdf:
        if not filename_pdf.lower().endswith(".pdf"):
            continue

        text_lines: List[str] = pdf_parser.get_text_lines_from_pdf(
            directory, filename_pdf
        )

        credit_card_billing.add_monthly_billing_data(text_lines)

        filename_txt_absolute: str = pdf_parser.get_filename_txt_absolute(
            directory, filename_pdf
        )

        pdf_parser.write_text_lines_to_file_in_dir(
            filename_txt_absolute, text_lines
        )

    credit_card_billing.write_to_directory(directory)


if __name__ == "__main__":
    main()
