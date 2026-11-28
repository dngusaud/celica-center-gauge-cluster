
import tkinter as tk
from tkinter import ttk, filedialog, messagebox

import pandas as pd
import matplotlib.pyplot as plt

from matplotlib.backends.backend_tkagg import (
    FigureCanvasTkAgg,
    NavigationToolbar2Tk
)


class DataLoggerViewer:

    # Fixed channel colours
    CHANNEL_COLORS = [
        "#2196F3",  # Blue
        "#FF9800",  # Orange
        "#4CAF50",  # Green
        "#F44336",  # Red
        "#9C27B0",  # Purple
        "#795548",  # Brown
        "#E91E63",  # Pink
        "#607D8B",  # Blue Grey
    ]

    def __init__(self, root):

        self.root = root
        self.root.title("ESP32 Data Logger Viewer")
        self.root.geometry("1100x700")

        self.df = None
        self.channel_vars = {}
        self.current_filename = None

        self.create_menu()
        self.create_ui()

    # ---------------------------------------------------------
    # MENU BAR
    # ---------------------------------------------------------

    def create_menu(self):

        menubar = tk.Menu(self.root)

        # File menu
        file_menu = tk.Menu(menubar, tearoff=0)

        file_menu.add_command(
            label="Import CSV...",
            command=self.import_csv
        )

        file_menu.add_command(
            label="Clear Logged Data...",
            command=self.clear_logged_data
        )

        file_menu.add_separator()

        file_menu.add_command(
            label="Exit",
            command=self.root.quit
        )

        menubar.add_cascade(
            label="File",
            menu=file_menu
        )

        self.root.config(menu=menubar)

    # ---------------------------------------------------------
    # GUI
    # ---------------------------------------------------------

    def create_ui(self):

        main = ttk.Frame(self.root)
        main.pack(fill=tk.BOTH, expand=True)

        # -----------------------------------------------------
        # Left panel
        # -----------------------------------------------------

        left = ttk.Frame(main, padding=10, width=200)
        left.pack(side=tk.LEFT, fill=tk.Y)

        left.pack_propagate(False)

        # Channel selection
        ttk.Label(
            left,
            text="Data Channels",
            font=("Arial", 11, "bold")
        ).pack(anchor=tk.W, pady=(5, 5))

        self.channel_frame = ttk.Frame(left)
        self.channel_frame.pack(fill=tk.X)

        # Plot style selection
        ttk.Label(
            left,
            text="Plot Style",
            font=("Arial", 11, "bold")
        ).pack(anchor=tk.W, pady=(20, 5))

        self.plot_style = tk.StringVar(value="lines")

        styles = [
            ("Lines", "lines"),
            ("Raw Points", "points"),
            ("Lines + Points", "both")
        ]

        for label, value in styles:

            ttk.Radiobutton(
                left,
                text=label,
                variable=self.plot_style,
                value=value,
                command=self.update_plot
            ).pack(anchor=tk.W)

        # -----------------------------------------------------
        # Right panel
        # -----------------------------------------------------

        right = ttk.Frame(main, padding=5)
        right.pack(side=tk.RIGHT, fill=tk.BOTH, expand=True)

        self.fig, self.ax = plt.subplots(figsize=(8, 5))

        self.ax.set_title("Data Logger")
        self.ax.set_xlabel("Time (ms)")
        self.ax.set_ylabel("Value")
        self.ax.grid(True)

        self.canvas = FigureCanvasTkAgg(
            self.fig,
            master=right
        )

        self.canvas.get_tk_widget().pack(
            fill=tk.BOTH,
            expand=True
        )

        # Matplotlib toolbar
        toolbar = NavigationToolbar2Tk(
            self.canvas,
            right
        )

        toolbar.update()

        # -----------------------------------------------------
        # Status bar
        # -----------------------------------------------------

        self.status = ttk.Label(
            self.root,
            text="No file loaded",
            relief=tk.SUNKEN,
            anchor=tk.W
        )

        self.status.pack(
            side=tk.BOTTOM,
            fill=tk.X
        )

    # ---------------------------------------------------------
    # CSV IMPORT
    # ---------------------------------------------------------

    def import_csv(self):

        filename = filedialog.askopenfilename(
            title="Import CSV Log",
            filetypes=[
                ("CSV Files", "*.csv"),
                ("All Files", "*.*")
            ]
        )

        if not filename:
            return

        try:

            df = pd.read_csv(filename)

            # Validate time column
            if "time_ms" not in df.columns:

                messagebox.showerror(
                    "Invalid CSV",
                    "Missing time_ms column"
                )

                return

            # Validate numeric data
            df["time_ms"] = pd.to_numeric(
                df["time_ms"],
                errors="coerce"
            )

            # Remove invalid time rows
            df = df.dropna(subset=["time_ms"])

            if df.empty:

                messagebox.showerror(
                    "Invalid CSV",
                    "No valid time data found"
                )

                return

            # Convert other columns to numeric
            for col in df.columns:

                if col != "time_ms":

                    df[col] = pd.to_numeric(
                        df[col],
                        errors="coerce"
                    )

            self.df = df
            self.current_filename = filename

            # -------------------------------------------------
            # Reset channel checkboxes
            # -------------------------------------------------

            for widget in self.channel_frame.winfo_children():
                widget.destroy()

            self.channel_vars.clear()

            # Create channel checkboxes
            for col in self.df.columns:

                if col == "time_ms":
                    continue

                if not pd.api.types.is_numeric_dtype(
                    self.df[col]
                ):
                    continue

                var = tk.BooleanVar(value=True)

                self.channel_vars[col] = var

                ttk.Checkbutton(
                    self.channel_frame,
                    text=col,
                    variable=var,
                    command=self.update_plot
                ).pack(anchor=tk.W)

            # -------------------------------------------------
            # Update status
            # -------------------------------------------------

            duration = (
                self.df["time_ms"].iloc[-1]
                - self.df["time_ms"].iloc[0]
            ) / 1000

            self.status.config(
                text=(
                    f"File: {filename} | "
                    f"Samples: {len(self.df)} | "
                    f"Duration: {duration:.2f} s"
                )
            )

            self.update_plot()

        except Exception as e:

            messagebox.showerror(
                "Import Error",
                str(e)
            )

    # ---------------------------------------------------------
    # CLEAR LOGGED DATA
    # ---------------------------------------------------------

    def clear_logged_data(self):

        filename = self.current_filename

        # If no file is currently loaded, ask user to select one
        if filename is None:

            filename = filedialog.askopenfilename(
                title="Select CSV Log to Clear",
                filetypes=[
                    ("CSV Files", "*.csv"),
                    ("All Files", "*.*")
                ]
            )

            if not filename:
                return

        # Confirmation popup
        confirm = messagebox.askyesno(
            "Clear Logged Data",
            f"Are you sure you want to clear all logged data?\n\n"
            f"File: {filename}\n\n"
            f"All data rows will be permanently deleted.\n"
            f"The CSV header will be preserved.",
            icon="warning"
        )

        if not confirm:
            return

        try:

            # Read original header
            with open(
                filename,
                "r",
                encoding="utf-8-sig",
                newline=""
            ) as file:

                header = file.readline()

            if not header.strip():

                messagebox.showerror(
                    "Clear Error",
                    "CSV file does not contain a valid header."
                )

                return

            # Rewrite file with header only
            with open(
                filename,
                "w",
                encoding="utf-8-sig",
                newline=""
            ) as file:

                file.write(header)

            # If this is the currently loaded file,
            # reset the viewer
            if self.current_filename == filename:

                self.df = None
                self.current_filename = None

                self.channel_vars.clear()

                for widget in self.channel_frame.winfo_children():
                    widget.destroy()

                # Clear plot
                self.ax.clear()

                self.ax.set_title("Data Logger")
                self.ax.set_xlabel("Time (ms)")
                self.ax.set_ylabel("Value")
                self.ax.grid(True)

                self.canvas.draw_idle()

                self.status.config(
                    text="Logged data cleared | No file loaded"
                )

            messagebox.showinfo(
                "Clear Complete",
                "Logged data successfully cleared.\n\n"
                "CSV header has been preserved."
            )

        except Exception as e:

            messagebox.showerror(
                "Clear Error",
                str(e)
            )

    # ---------------------------------------------------------
    # PLOT UPDATE
    # ---------------------------------------------------------

    def update_plot(self):

        if self.df is None:
            return

        self.ax.clear()

        style = self.plot_style.get()

        # Plot selected channels
        for index, (channel, var) in enumerate(
            self.channel_vars.items()
        ):

            if not var.get():
                continue

            x = self.df["time_ms"]
            y = self.df[channel]

            # Fixed colour based on channel index
            color = self.CHANNEL_COLORS[
                index % len(self.CHANNEL_COLORS)
            ]

            if style == "lines":

                self.ax.plot(
                    x,
                    y,
                    label=channel,
                    color=color,
                    linewidth=1
                )

            elif style == "points":

                self.ax.scatter(
                    x,
                    y,
                    label=channel,
                    color=color,
                    s=8
                )

            elif style == "both":

                self.ax.plot(
                    x,
                    y,
                    label=channel,
                    color=color,
                    linewidth=0.8,
                    marker="o",
                    markersize=3
                )

        # Axis settings
        self.ax.set_title("Data Logger")
        self.ax.set_xlabel("Time (ms)")
        self.ax.set_ylabel("Value")

        self.ax.grid(True)

        if any(
            var.get()
            for var in self.channel_vars.values()
        ):

            self.ax.legend()

        self.fig.tight_layout()

        self.canvas.draw_idle()


# =============================================================
# MAIN
# =============================================================

if __name__ == "__main__":

    root = tk.Tk()

    app = DataLoggerViewer(root)

    root.mainloop()
