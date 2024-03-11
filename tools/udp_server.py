import socket
import zlib
import struct

def decompress_and_parse(data):
    # 解压缩数据
    decompressed_data = zlib.decompress(data)

    # 将解压缩后的数据解析为float列表（假设是小端序）
    floats = [struct.unpack('<f', decompressed_data[i:i+4])[0] for i in range(0, len(decompressed_data), 4)]

    return floats

def udp_server(host='0.0.0.0', port=12121):
    # 创建UDP socket
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

    # 绑定socket到地址
    server_address = (host, port)
    sock.bind(server_address)
    print(f"UDP server listening on {host}:{port}")

    while True:
        print("\nWaiting to receive message...")
        data, address = sock.recvfrom(4096)

        print(f"Received {len(data)} bytes from {address}")
        try:
            # 解压缩并解析接收到的数据
            floats = decompress_and_parse(data)
            print("Decompressed and parsed floats:", floats)
        except zlib.error as e:
            print("Error decompressing data:", e)
        except struct.error as e:
            print("Error parsing floats:", e)

if __name__ == "__main__":
    udp_server()
