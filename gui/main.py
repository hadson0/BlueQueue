import tkinter as tk
import serial

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
    
    response = ser.readline()
    print(response)
    
    if response != b'':
        response = response.strip().decode('utf-8')
        aux = response.split(";")
        if len(aux) != 3:
            return
        
        queue_str, current, total = aux
        queue = [number for number in queue_str]

        team_queue_label.config(text="\n".join(queue))                                
        current_team.config(text=current)
        total_number.config(text=total)

        ser.close()
        root.after(100, update_interface)               # Atualiza a interface a cada 100ms

if __name__ == '__main__':
    update_interface()
    root.mainloop()