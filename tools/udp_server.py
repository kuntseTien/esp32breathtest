import socket
import zlib
import struct
import numpy as np
from datetime import datetime

def decompress_and_parse(data):
    # 解压缩数据
    decompressed_data = zlib.decompress(data)

    # 将解压缩后的数据解析为float列表（假设是小端序）
    floats = [struct.unpack('<f', decompressed_data[i:i+4])[0] for i in range(0, len(decompressed_data), 4)]

    return floats

def direct_parse(data):
    floats_array = np.frombuffer(data, dtype=np.float32)
    print("首尾數據為:", floats_array[0], ", ", floats_array[-1])
    print(" ")

def udp_server(host='0.0.0.0', port=12121):
    COMPRESSED_FLAG = True
    
    # 创建UDP socket
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

    # 绑定socket到地址
    server_address = (host, port)
    sock.bind(server_address)
    print(f"UDP server listening on {host}:{port}")
    print("\nWaiting to receive message...")
    while True:
        data, address = sock.recvfrom(8192)
        recv_time = datetime.now()
        print(f"Received time: {recv_time.strftime('%Y-%m-%d %H:%M:%S.%f')}")
        # print(f"Received {len(data)} bytes from {address}")
        
        try:
            if COMPRESSED_FLAG == True:
                floats = decompress_and_parse(data)
            else:
                floats = direct_parse(data)
        except zlib.error as e:
            print("Error decompressing data:", e)
            COMPRESSED_FLAG = False
        except struct.error as e:
            print("Error parsing floats:", e)

if __name__ == "__main__":
    udp_server()
