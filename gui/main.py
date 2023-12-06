import tkinter as tk
import serial

SERIAL_PORT = '/dev/ttyUSB0'
SERIAL_BAUDRATE = 9600

root = tk.Tk()
root.title("Sistema de Fila")

fila_label = tk.Label(root, text="Próximos times:")
fila_label.pack()

fila_numbers = ['---'] * 5
fila_numbers_label = tk.Label(root, text="\n".join(fila_numbers))
fila_numbers_label.pack()

chamado_label = tk.Label(root, text="Último número chamado: ")
chamado_label.pack()

chamado_number = tk.Label(root)
chamado_number.pack()

total_label = tk.Label(root, text="Último número chamado: ")
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
        queue_str, chamado, total = aux
        
        queue = [number for number in queue_str]
        fila_numbers_label.config(text="\n".join(queue))
                                
        chamado_number.config(text=chamado)

        total_number.config(text=total)

        ser.close()
        root.after(1, update_interface)

if __name__ == '__main__':
    update_interface()
    root.mainloop()