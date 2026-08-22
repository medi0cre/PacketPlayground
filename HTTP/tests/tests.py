import csv
import socket

host = "127.0.0.1"
port = 8080
csv_file = "tests.csv"
chunk_size = 4096

def send_request(request):
    with socket.create_connection((host, port), timeout = 5) as sock:
        sock.sendall(request.encode("ascii"))
        sock.shutdown(socket.SHUT_WR)

        response = b""
        while True:
            chunk = sock.recv(chunk_size)
            if not chunk:
                break
            response += chunk

        return response

def main():

    with open(csv_file, newline="", encoding = "utf-8") as file:
        reader = csv.reader(file)

        for test_id, description, request in reader:
            request = request.replace("\\r\\n", "\r\n")
            print(f"\nTest {test_id}: {description}")
            print(f"Request: {request!r}")

            try:
                response = send_request(request)
                print("Response:")
                print(response.decode("iso-8859-1"))

            except socket.timeout:
                print("ERROR: Server timed out")

            except ConnectionRefusedError:
                print("ERROR: Could not connect to 127.0.0.1:8080")

            except Exception as e:
                print(f"ERROR: {e}")

if __name__ == "__main__":
    main()
