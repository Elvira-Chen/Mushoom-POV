import socket
from calendar import c
import numpy as np
import os
import math
from PIL import Image

# 配置
# Frame = 5 #指定帧数量
NUMPIXELS = 50  # 单边LED数量
Div = 320  # 1圈分割数
Bright = 60  # LED亮度
Led0Bright = 15  # 中心LED的亮度

gif_file_name = "test_img/apple.gif"
im = Image.open(gif_file_name)
Frame = im.n_frames


tcp_client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server_ip = "192.168.31.96"
server_sport = 3333
tcp_client_socket.connect((server_ip, server_sport))


def tcpsend(send_data: bytes):

    print("发送数据: ", len(send_data))
    tcp_client_socket.send(send_data)
    
    recvData = tcp_client_socket.recv(len(send_data))
    
    print('收到数据: ', len(recvData))
    # total_recv_bytes=len(recvData)
    # while len(send_data)-total_recv_bytes > 0:

    #     recvData = tcp_client_socket.recv(len(send_data))
    #     total_recv_bytes+=len(recvData)
    #     print('收到数据: ', len(recvData))
    #     print("总收到的数据量: ", total_recv_bytes)


# 画像変換関数
def polarConv(imgOrgin, frame):

    h = imgOrgin.height  # 帧尺寸
    w = imgOrgin.width

    # 画像縮小
    imgRedu = imgOrgin.resize(
        (math.floor((NUMPIXELS * 2 - 1)/h * w), NUMPIXELS * 2 - 1)).rotate(180)
    # imgRedu.save(str(frame)+'.png')   #输出缩小后的原图像
    imgArray = np.array(imgRedu)
    # 縮小画像中心座標
    h2 = imgRedu.height
    w2 = imgRedu.width
    wC = math.floor(w2 / 2)
    hC = math.floor(h2 / 2)

    # 極座標変換画像準備
    imgPolar = Image.new('RGB', (NUMPIXELS, Div))

    # 極座標変換
    for j in range(0, Div):
        for i in range(0, hC+1):
            # 座標色取得
            rP = int(imgArray[hC + math.ceil(i * math.cos(2*math.pi/Div*j)),
                              wC - math.ceil(i * math.sin(2*math.pi/Div*j)), 0] * ((100 - Led0Bright) / NUMPIXELS * i + Led0Bright) / 100 * Bright / 100)
            gP = int(imgArray[hC + math.ceil(i * math.cos(2*math.pi/Div*j)),
                              wC - math.ceil(i * math.sin(2*math.pi/Div*j)), 1] * ((100 - Led0Bright) / NUMPIXELS * i + Led0Bright) / 100 * Bright / 100)
            bP = int(imgArray[hC + math.ceil(i * math.cos(2*math.pi/Div*j)),
                              wC - math.ceil(i * math.sin(2*math.pi/Div*j)), 2] * ((100 - Led0Bright) / NUMPIXELS * i + Led0Bright) / 100 * Bright / 100)
            imgPolar.putpixel((i, j), (rP, gP, bP))

    imgPolar.save(str(frame)+'.jpg', quality=95, optimize=True)  # 输出极坐标变换后的图像

    # 构造发送数据
    data_to_send = bytes()
    for j in range(0, Div):
        for i in range(0, hC+1):
            by = bytes(imgPolar.getpixel((i, j)))
            data_to_send += by
    # print(data_to_send)
    print("data len:", len(data_to_send))
    tcpsend(data_to_send)


if __name__ == "__main__":
    for i in range(Frame):
        # 输出每一帧
        frame = im.convert('RGBA')  # 如果是RGB的话，有的透明背景GIF不兼容
        polarConv(frame, i)
        if i != Frame-1:
            im.seek(im.tell()+1)
        # break

    tcp_client_socket.close()
