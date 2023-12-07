import tkinter as tk
import serial
import re

SERIAL_PORT = '/dev/ttyUSB0'
SERIAL_BAUDRATE = 9600

root = tk.Tk()
root.title("Sistema de Fila Eletrônica")

current_team_label = tk.Label(root, text="Time sendo atendido: ")
current_team_label.pack()

current_team = tk.Label(root)
current_team.pack()

queue_label = tk.Label(root, text="Próximos times:")
queue_label.pack()

team_queue = ['---'] * 5
team_queue_label = tk.Label(root, text="\n".join(team_queue))
team_queue_label.pack()

total_label = tk.Label(root, text="Times cadastrados: ")
total_label.pack()

total_number = tk.Label(root)
total_number.pack()


def update_interface():
    ser = serial.Serial(SERIAL_PORT, SERIAL_BAUDRATE, timeout=0.1)
    ser.write(b'd')
    
    try:
        response = ser.readline()
    except:
        return
    print(response)

    root.after(100, update_interface)
    if response == b'':
        return
    
    response = response.decode('utf-8')

    pattern = r"data:(.*?);(.*?);(.*?)"
    match = re.match(pattern, response)

    if match:
        queue = list(match.group(1))
        current = match.group(2)
        total = match.group(3)

        team_queue_label.config(text="\n".join(queue))                                
        current_team.config(text=current)
        total_number.config(text=total)

        ser.close()

if __name__ == '__main__':
    update_interface()
    root.mainloop()