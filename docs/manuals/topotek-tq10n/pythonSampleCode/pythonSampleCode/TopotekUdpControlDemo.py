#python3

import tkinter as tk
from tkinter import scrolledtext
import socket
import threading

class UDPToolDemo:
    def __init__(self, master):
        self.master = master
        self.master.title("Topotek UDP send/receive Demo v1.0.0")
        self.create_widgets()
        self.udp_socket = None
        self.listening = False

    def create_widgets(self):
        frame_send = tk.Frame(self.master)
        frame_send.pack(padx=10, pady=5, fill="x")

        tk.Label(frame_send, text="destination IP:").pack(side="left")
        self.entry_ip = tk.Entry(frame_send, width=15)
        self.entry_ip.pack(side="left", padx=5)
        self.entry_ip.insert(0, "192.168.144.108")

        tk.Label(frame_send, text="destination port:").pack(side="left")
        self.entry_port = tk.Entry(frame_send, width=5)
        self.entry_port.pack(side="left", padx=5)
        self.entry_port.insert(0, "9003")

        tk.Label(frame_send, text="Data to be sent:").pack(side="left")
        self.entry_data = tk.Entry(frame_send, width=40)
        self.entry_data.pack(side="left", padx=5)
        self.entry_data.insert(0, "#TPUD2wCAP013E") #snap
        #self.entry_data.insert(0, "#TPUD2wREC0A54") #record
        #self.entry_data.insert(0, "#TPUG2wPTZ016B") #up
        #self.entry_data.insert(0, "#TPUG2wPTZ026C") #down
        #self.entry_data.insert(0, "#TPUG2wPTZ036D") #left
        #self.entry_data.insert(0, "#TPUG2wPTZ046E") #right
        #self.entry_data.insert(0, "#TPUG2wPTZ006A") #stop
        #self.entry_data.insert(0, "#TPUG2wPTZ056F") #return home

        #self.entry_data.insert(0, "23 54 50 55 44 32 77 43 41 50 30 31 33 45")  #snap   hex mode

        self.hex_var = tk.BooleanVar(value=False)
        self.separate_var = tk.BooleanVar(value=False)
        self.hex_check = tk.Checkbutton(frame_send, text="Hex", variable=self.hex_var)
        self.hex_check.pack(side="left", padx=5)
        self.separate_check = tk.Checkbutton(frame_send, text="Separate", variable=self.separate_var)
        self.separate_check.pack(side="left", padx=5)

        self.btn_send = tk.Button(frame_send, text="Send", command=self.send_udp_data)
        self.btn_send.pack(side="left", padx=5)

        frame_listen = tk.Frame(self.master)
        frame_listen.pack(padx=10, pady=5, fill="x")

        tk.Label(frame_listen, text="listen to Port:").pack(side="left")
        self.entry_listen_port = tk.Entry(frame_listen, width=5)
        self.entry_listen_port.pack(side="left", padx=5)
        self.entry_listen_port.insert(0, "9004")

        self.btn_listen = tk.Button(frame_listen, text="Start Listen", command=self.start_listen)
        self.btn_listen.pack(side="left", padx=5)
        self.btn_stop = tk.Button(frame_listen, text="Stop Listen", command=self.stop_listen, state="disabled")
        self.btn_stop.pack(side="left", padx=5)

        self.txt_receive = scrolledtext.ScrolledText(self.master, width=60, height=15, state="disabled")
        self.txt_receive.pack(padx=10, pady=5)

    def send_udp_data(self):
        ip = self.entry_ip.get()
        try:
            port = int(self.entry_port.get())
        except ValueError:
            self.append_text("destination port is invalid\n")
            return

        raw_data = self.entry_data.get()
        if self.hex_var.get():
            try:
                if self.separate_var.get():
                    data_bytes = bytes(int(b, 16) for b in raw_data.split())
                else:
                    if len(raw_data) % 2 != 0:
                        self.append_text("Invalid hex string: length must be even\n")
                        return
                    data_bytes = bytes.fromhex(raw_data)
            except ValueError as e:
                self.append_text(f"Invalid hex string: {e}\n")
                return
        else:
            data_bytes = raw_data.encode("utf-8")
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            sock.sendto(data_bytes, (ip, port))
            sock.close()
            self.append_text(f"Sent Successful to {ip}:{port} -> {raw_data}\n")
        except Exception as e:
            self.append_text(f"Sent Failed: {e}\n")

    def start_listen(self):
        try:
            port = int(self.entry_listen_port.get())
        except ValueError:
            self.append_text("Listen Port is invalid\n")
            return

        self.udp_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.udp_socket.bind(("0.0.0.0", port))
        self.listening = True
        self.btn_listen.config(state="disabled")
        self.btn_stop.config(state="normal")
        self.append_text(f"Start to listen port {port}\n")
        threading.Thread(target=self.listen_loop, daemon=True).start()

    def listen_loop(self):
        while self.listening:
            try:
                data, addr = self.udp_socket.recvfrom(4096)
                message = data.decode("utf-8", errors="replace")
                self.append_text(f"Data from {addr}: {message}\n")
            except Exception as e:
                if self.listening:
                    self.append_text(f"Failed: {e}\n")
                break

    def stop_listen(self):
        self.listening = False
        if self.udp_socket:
            self.udp_socket.close()
            self.udp_socket = None
        self.btn_listen.config(state="normal")
        self.btn_stop.config(state="disabled")
        self.append_text("Stop listen\n")

    def append_text(self, text):
        self.txt_receive.config(state="normal")
        self.txt_receive.insert(tk.END, text)
        self.txt_receive.see(tk.END)
        self.txt_receive.config(state="disabled")

if __name__ == "__main__":
    root = tk.Tk()
    app = UDPToolDemo(root)
    root.mainloop()
