#!/usr/bin/env python3

# from tkinter import filedialog
import tkinter as tk
import tkinter.filedialog


def main():
    root = tk.Tk()
    root.withdraw()
    folder_selected = tk.filedialog.askdirectory()

    print(folder_selected)

    # gui to get folder?

    # iterate over all pdfs in folder

    # create attach each pdf to an excel file going by year


if __name__ == "__main__":
    main()
