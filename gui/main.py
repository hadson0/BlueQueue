import customtkinter as ctk
import serial
import re

SERIAL_PORT = '/dev/ttyUSB0'
SERIAL_BAUDRATE = 9600
DELAY = 500
BASE_FONT_SIZE = 26


class ScrollableListFrame(ctk.CTkScrollableFrame):
    def __init__(self, master, **kwargs):
        super().__init__(master, **kwargs)
        self.grid_columnconfigure(0, weight=1)
        self.label_list = []

    def update_list(self, team_list):
        # Remove os elemenos que não estão na lista de times
        for label in self.label_list:
            if label.cget("text") not in team_list:
                label.destroy()
                self.label_list.remove(label)

        # Adiciona os novos elementos
        for item in team_list:
            if item not in [label.cget("text") for label in self.label_list]:
                label = ctk.CTkLabel(
                    self, text=item, padx=5, 
                    font=ctk.CTkFont(family='Lato', size=BASE_FONT_SIZE)
                    )
            label.grid(row=len(self.label_list), column=0, pady=10, sticky="we")
            self.label_list.append(label)

class QueueInfoFrame(ctk.CTkFrame):
    def __init__(self, master, **kwargs):
        super().__init__(master, **kwargs)
        self.grid_columnconfigure(0, weight=1)

        self._current_team = ctk.StringVar(value="-")
        self._total_teams = ctk.StringVar(value="-")
        self._queue_size = ctk.StringVar(value="-")

        self._current_label = ctk.CTkLabel(
            self, text="Vez da Equipe:", font=ctk.CTkFont(family='Lato', size=BASE_FONT_SIZE + 4)
            )
        self._current_label.grid(row=0, column=0, padx=15, pady=(25, 5), sticky="we")
        self._current_label_number_label = ctk.CTkLabel(
            self, textvariable=self._current_team, width=100, corner_radius=10,
            font=ctk.CTkFont(family='Lato', size=BASE_FONT_SIZE * 4, weight='bold')
            )
        self._current_label_number_label.grid(row=1, column=0, padx=0, pady=0, sticky="n")

        self._queue_size_label = ctk.CTkLabel(
            self, text="Equipes na fila:", font=ctk.CTkFont(family='Lato', size=BASE_FONT_SIZE + 4)
            )
        self._queue_size_label.grid(row=2, column=0, padx=15, pady=(25, 5), sticky="we")
        self._queue_size_number_label = ctk.CTkLabel(
            self, text="10", textvariable=self._queue_size, font=ctk.CTkFont(family='Lato', size=BASE_FONT_SIZE + 20, weight='bold')
            )
        self._queue_size_number_label.grid(row=3, column=0, padx=0, pady=0, sticky="n")

        self._total_label = ctk.CTkLabel(
            self, text="Equipes cadastradas:", font=ctk.CTkFont(family='Lato', size=BASE_FONT_SIZE + 4)
            )
        self._total_label.grid(row=4, column=0, padx=15, pady=(25, 5), sticky="we")
        self._total_label_number_label = ctk.CTkLabel(
            self, textvariable=self._total_teams, font=ctk.CTkFont(family='Lato', size=BASE_FONT_SIZE + 20, weight='bold')
            )
        self._total_label_number_label.grid(row=5, column=0, padx=0, pady=0, sticky="n")

    def update_info(self, current_team, queue_size, total_teams):
        if current_team == "/":
            current_team = "-"

        self._current_team.set(current_team)
        self._queue_size.set(queue_size)
        self._total_teams.set(total_teams)

class App(ctk.CTk):
    def __init__(self):
        super().__init__()

        self.title("Sistema de Fila Eletrônica")
        self.geometry(f'{800}x{500}')
        self.minsize(800, 500)
        self.grid_rowconfigure(1, weight=1)
        self.columnconfigure(0, weight=1)
        self.columnconfigure(1, weight=1)

        self.info_label = ctk.CTkLabel(
            self, text="Informações da fila:", anchor="s",
            font=ctk.CTkFont(family='Lato', size=BASE_FONT_SIZE + 6, weight='bold')
            )
        self.info_label.grid(row=0, column=0, padx=0, pady=(20, 0), sticky="s")

        self.queue_info_frame = QueueInfoFrame(self, corner_radius=10)
        self.queue_info_frame.grid(row=1, column=0, padx=(20, 10), pady=(5, 25), sticky="nsew")

        self.queue_label = ctk.CTkLabel(
            self, text="Próximas equipes:", anchor="s",
            font=ctk.CTkFont(family='Lato', size=BASE_FONT_SIZE + 6, weight='bold')
            )
        self.queue_label.grid(row=0, column=1, padx=0, pady=(20, 0), sticky="s")

        self.scrollable_list_frame = ScrollableListFrame(master=self, corner_radius=10)
        self.scrollable_list_frame.grid(row=1, column=1, padx=(10, 20), pady=(5, 25), sticky="nsew")

        self.update_interface()

    def update_interface(self):    
        try:
            ser = serial.Serial(SERIAL_PORT, SERIAL_BAUDRATE, timeout=0.1)
            ser.write(b'd')
            response = ser.readline()
            response = response.decode('utf-8')
        except:
            self.after(DELAY, self.update_interface)
            return

        # print(response)

        pattern = r"data:(.*?);(.*?);(.*?);(.*?);"
        match = re.match(pattern, response)

        if match:
            queue = [f"Equipe {team}" for team in list(match.group(1))]
            current = match.group(2)
            queue_size = match.group(3)
            total_teams = match.group(4)

            self.scrollable_list_frame.update_list(queue)
            self.queue_info_frame.update_info(current, queue_size, total_teams)

            ser.close()
        self.after(DELAY, self.update_interface)

if __name__ == "__main__":
    ctk.set_appearance_mode("light")
    app = App()
    app.mainloop()