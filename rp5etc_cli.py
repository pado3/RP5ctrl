#!/usr/bin/env python3.8
# TinkerboardのspidevがPython3.5.3でNG、ASUS.GPIOがPython3.13でNG、よって3.8
#
# r1.2 2025/02/08 ウェイト見直し（RPi5の電源ダブル送信のところで前後間違い）
# r1.1 2025/02/05 RPi5の電源はマスタ側からダブルクリック
# r1.0 2025/02/01 initial release
# Raspberry Pi 5の電源とAV機器の簡単なコントローラ by pado@mstdn.jp
#
import spidev
import sys
import time


def demo(args):
    spi = spidev.SpiDev()
    spi.open(2, 0)  # RPiでは(0, 0)
    spi.mode = 0b01
    spi.max_speed_hz = 9600
    CMD_hex = int("0", 16)
    for arg in args[1:]:
        try:
            CMD_hex = int(arg, 16)
            spi.xfer([CMD_hex])
            print("CMD {:x} is send.".format(CMD_hex))
        except Exception:
            print("This arg is not HEX value: {}".format(arg))
        time.sleep(0.5)     # チャタリング予防
        if CMD_hex == 0x1:  # RPi5の場合、マスタ側でダブルクリックする
            print("RPi POWER ON/OFF")
            spi.xfer([CMD_hex])
            print("CMD {:x} is send.".format(CMD_hex))
            time.sleep(0.5)     # 連続送信のためのウェイト
    spi.close()


# お約束
if __name__ == "__main__":
    # Pythonスクリプトに渡されたコマンドライン引数のリスト。
    # args[0] はスクリプトの名前, len(args)がpython3に渡される引数の数
    args = sys.argv
    if len(args) == 0:
        print("USAGE: {} senddata(s) in HEX.".format(args[0]))
    else:
        demo(args)
