import subprocess
import threading
import queue
import tkinter as tk
from tkinter import scrolledtext, messagebox

# Path to the Athena engine binary
ATHENA_PATH = "./build/src/athena"

class AthenaGUI:
    def __init__(self, root):
        self.root = root
        self.root.title("Athena 4-Player Chess GUI")

        # Engine process and communication
        self.process = None
        self.output_queue = queue.Queue()

        # --- UI Layout ---

        # Left: ASCII board display
        self.board_display = tk.Text(
            root,
            width=32,
            height=18,
            font=("Courier", 16),
            state="disabled"
        )
        self.board_display.grid(row=0, column=0, padx=10, pady=10, sticky="nsew")

        # Right: Engine log
        self.log_display = scrolledtext.ScrolledText(
            root,
            width=70,
            height=20,
            font=("Consolas", 11)
        )
        self.log_display.grid(row=0, column=1, padx=10, pady=10, sticky="nsew")

        # Bottom: Command entry and buttons
        bottom_frame = tk.Frame(root)
        bottom_frame.grid(row=1, column=0, columnspan=2, sticky="ew", padx=10, pady=5)

        self.entry = tk.Entry(bottom_frame, width=80)
        self.entry.grid(row=0, column=0, columnspan=5, sticky="ew")
        self.entry.bind("<Return>", self.send_command_from_entry)

        tk.Button(bottom_frame, text="uci", command=self.send_uci).grid(row=1, column=0, pady=5, sticky="ew")
        tk.Button(bottom_frame, text="isready", command=self.send_isready).grid(row=1, column=1, pady=5, sticky="ew")
        tk.Button(bottom_frame, text="ucinewgame", command=self.send_ucinewgame).grid(row=1, column=2, pady=5, sticky="ew")
        tk.Button(bottom_frame, text="startpos", command=self.set_startpos).grid(row=1, column=3, pady=5, sticky="ew")
        tk.Button(bottom_frame, text="go depth 10", command=self.go_depth_10).grid(row=1, column=4, pady=5, sticky="ew")

        for i in range(5):
            bottom_frame.columnconfigure(i, weight=1)

        root.columnconfigure(0, weight=0)
        root.columnconfigure(1, weight=1)
        root.rowconfigure(0, weight=1)

        # Start engine and output polling
        self.start_engine()
        self.root.after(50, self.poll_engine_output)
        self.root.protocol("WM_DELETE_WINDOW", self.on_close)

    def start_engine(self):
        try:
            self.process = subprocess.Popen(
                [ATHENA_PATH],
                stdin=subprocess.PIPE,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
                bufsize=1
            )
        except Exception as e:
            messagebox.showerror("Error", f"Could not start Athena engine:\n{e}")
            self.process = None
            return

        threading.Thread(target=self.read_engine_output, daemon=True).start()
        self.log(f"Started Athena engine: {ATHENA_PATH}\n")

    def read_engine_output(self):
        if not self.process or not self.process.stdout:
            return
        for line in self.process.stdout:
            self.output_queue.put(line)

    def poll_engine_output(self):
        while not self.output_queue.empty():
            line = self.output_queue.get_nowait()
            self.handle_engine_output(line)
        self.root.after(50, self.poll_engine_output)

    def on_close(self):
        if self.process and self.process.poll() is None:
            try:
                self.send_raw_command("quit")
                self.process.terminate()
            except Exception:
                pass
        self.root.destroy()

    # --- Command sending ---

    def send_raw_command(self, cmd):
        if not self.process or not self.process.stdin:
            self.log("Engine is not running; cannot send command.\n")
            return
        try:
            self.process.stdin.write(cmd + "\n")
            self.process.stdin.flush()
            self.log(f">>> {cmd}\n")
        except Exception as e:
            self.log(f"Failed to send command: {e}\n")

    def send_command_from_entry(self, event=None):
        cmd = self.entry.get().strip()
        if not cmd:
            return
        self.entry.delete(0, tk.END)
        self.send_raw_command(cmd)

    def send_uci(self):
        self.send_raw_command("uci")

    def send_isready(self):
        self.send_raw_command("isready")

    def send_ucinewgame(self):
        self.send_raw_command("ucinewgame")

    def set_startpos(self):
        self.send_raw_command("position startpos")
        self.send_raw_command("d")

    def go_depth_10(self):
        self.send_raw_command("go depth 10")

    # --- Output handling ---

    def handle_engine_output(self, line):
        self.log(line)
        stripped = line.strip()
        if stripped.startswith("FEN4:"):
            fen_board = stripped.split(":", 1)[1].strip()
            self.update_board_from_fen(fen_board)

    def update_board_from_fen(self, fen4_board):
        ascii_board = fen4_to_ascii_board(fen4_board)
        self.board_display.config(state="normal")
        self.board_display.delete("1.0", tk.END)
        self.board_display.insert(tk.END, ascii_board)
        self.board_display.config(state="disabled")

    def log(self, text):
        self.log_display.insert(tk.END, text)
        self.log_display.see(tk.END)

# --- FEN4 to ASCII board helper ---

def fen4_to_ascii_board(fen4_board):
    """
    Convert a FEN4 board section (14 ranks separated by '/')
    into a 14x14 ASCII representation for the GUI.
    """
    ranks = fen4_board.split("/")
    if len(ranks) != 14:
        return "(invalid FEN4: must contain 14 ranks)\n"

    ascii_lines = []
    rank_num = 14
    for rank in ranks:
        parts = rank.split(",")
        expanded_row = []
        for item in parts:
            if item.isdigit():
                expanded_row.extend(["."] * int(item))
            else:
                expanded_row.append(item)
        if len(expanded_row) != 14:
            return f"(invalid FEN4 row length at rank {rank_num})\n"
        ascii_lines.append(f"{rank_num:2d}  " + " ".join(expanded_row))
        rank_num -= 1

    files = " ".join([chr(ord('a') + i) for i in range(14)])
    ascii_lines.append(f"    {files}")
    return "\n".join(ascii_lines) + "\n"

if __name__ == "__main__":
    root = tk.Tk()
    app = AthenaGUI(root)
    root.mainloop()