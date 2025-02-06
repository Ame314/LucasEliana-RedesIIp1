import socket

# Configuración básica del servidor
SERVER_HOST = "0.0.0.0"  # Escucha en todas las interfaces de red disponibles
SERVER_PORT = 12345  # Puerto donde el servidor estará escuchando

# Crear el socket del servidor
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.bind((SERVER_HOST, SERVER_PORT))  # Asocia el socket al host y puerto
sock.listen(5)  # Permite hasta 5 conexiones en cola

print(f"Servidor activo en {SERVER_HOST}:{SERVER_PORT}, esperando conexiones...")

while True:
    # Espera una nueva conexión
    client_conn, client_addr = sock.accept()
    print(f"Cliente conectado desde: {client_addr}")

    while True:
        try:
            # Recibe datos del cliente
            received_data = client_conn.recv(1024).decode()
            if not received_data:
                print(f"El cliente {client_addr} se ha desconectado")
                break  # Sale del bucle si no hay más datos

            print(f"Mensaje recibido: {received_data}")

            # Procesa el mensaje recibido y genera una respuesta
            if "LED ENCENDIDO" in received_data:
                response = "Confirmación: LED está ahora encendido"
            elif "LED APAGADO" in received_data:
                response = "Confirmación: LED está ahora apagado"
            else:
                response = "Error: Comando no reconocido"

            # Envía la respuesta al cliente
            client_conn.sendall(response.encode())

        except ConnectionResetError:
            print(f"El cliente {client_addr} cerró la conexión inesperadamente")
            break

    client_conn.close()  # Cierra la conexión con el cliente actual