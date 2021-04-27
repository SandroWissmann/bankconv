#!/usr/bin/env python3

# from tkinter import filedialog
import tkinter as tk
import tkinter.filedialog

import os


def main():
    # get directory
    root = tk.Tk()
    root.withdraw()
    directory = tk.filedialog.askdirectory()

    print(directory)

    # iterate over all pdf files in folder
    for filename in os.listdir(directory):
        if filename.lower().endswith(".pdf"):
            print(os.path.join(directory, filename))

    # read out data from each pdf file. check if it is a table already?

    # create attach each pdf to an excel file going by year


if __name__ == "__main__":
    main()
