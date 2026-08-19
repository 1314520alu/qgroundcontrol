import re
import struct
from topotekcmdparse import build_command  # assume build_command is implemented in that module

'''
    Please note:

    - For responses from commands like `#tpDPDwTMP` and `#tPDP16TMP` (used in 12.1 and 12.2),
    the returned coordinates are based on a resolution of 320x256.
    You will need to scale these appropriately if mapping them to your display.

    - For point and region temperature measurements (12.3), the coordinates (e.g., XXX, YYY)
    are based on the full protocol resolution of 640x512. To convert them to display coordinates
    (e.g., for 1920x1080 screens), use:
 
    x_disp, y_disp = from_protocol_coords(px, py, display_width, display_height)

 '''
# Fixed protocol resolution
PROTOCOL_WIDTH = 640
PROTOCOL_HEIGHT = 512


def to_protocol_coords(x: float, y: float, disp_w: int, disp_h: int) -> tuple[int,int]:
    """
    Convert display coordinates (x, y) to protocol coordinates (0-640, 0-512).
    """
    px = int(round(x * PROTOCOL_WIDTH / disp_w))
    py = int(round(y * PROTOCOL_HEIGHT / disp_h))
    return px, py


def to_protocol_region(x: float, y: float, w: float, h: float,
                       disp_w: int, disp_h: int) -> tuple[int,int,int,int]:
    """
    Convert display region (x, y, w, h) to protocol region tuple (px, py, pw, ph).
    """
    px, py = to_protocol_coords(x, y, disp_w, disp_h)
    pw = int(round(w * PROTOCOL_WIDTH / disp_w))
    ph = int(round(h * PROTOCOL_HEIGHT / disp_h))
    return px, py, pw, ph


def from_protocol_coords(px: int, py: int, disp_w: int, disp_h: int) -> tuple[float,float]:
    """
    Map protocol coordinates (px, py) back to display coordinates (x, y).
    """
    x = px * disp_w / PROTOCOL_WIDTH
    y = py * disp_h / PROTOCOL_HEIGHT
    return x, y


def build_region_temp_query(x: float, y: float, w: float, h: float,
                            disp_w: int, disp_h: int,
                            address1: str = 'P', address2: str = 'D') -> str:
    """
    Create a region temperature query command.
    First convert display region (x, y, w, h) to protocol space 640x512.
    If w and h are both zero, it is treated as a point temperature query.
    """
    px, py, pw, ph = to_protocol_region(x, y, w, h, disp_w, disp_h)
    if not (0 <= px <= PROTOCOL_WIDTH and 0 <= py <= PROTOCOL_HEIGHT):
        raise ValueError("Converted top-left point is out of protocol range")
    if not (0 <= pw <= 64 and 0 <= ph <= 64):
        raise ValueError("Converted region width/height exceeds max 64")
    data = f"{px:03d}{py:03d}{pw:02d}{ph:02d}"
    return build_command(
        frame_header='#tp', address_bit1=address1, address_bit2=address2,
        control_bit='r', identifier_bit='TMP', data=data,
        data_mode='ASCII', input_space_separate=False,
        output_format='ASCII', output_space_separate=False
    )


if __name__ == '__main__':
    disp_w, disp_h = 1920, 1080
    # Region temperature query example
    x = 960
    y = 540
    w = 60  # if point temperature query at (x, y), set w=0 h=0  
    h = 60
    # Address mapping:
    # "U — UART", "M — Lens", "D — System and Image", "E — Sub-device",
    # "P — Network", "G — Gimbal"
    # address1 is the source address: use 'P' for network, 'U' for serial.
    address_bit1 = 'P'
    address_bit2 = 'D'
    cmd_data = build_region_temp_query(x, y, w, h, disp_w, disp_h, address_bit1, address_bit2)
    print('cmd data:', cmd_data)

    '''
    Please note:

    - For responses from commands like `#tpDPDwTMP` and `#tPDP16TMP` (used in 12.1 and 12.2),
    the returned coordinates are based on a resolution of 320x256.
    You will need to scale these appropriately if mapping them to your display.

    - For point and region temperature measurements (12.3), the coordinates (e.g., XXX, YYY)
    are based on the full protocol resolution of 640x512. To convert them to display coordinates
    (e.g., for 1920x1080 screens), use:
 
    x_disp, y_disp = from_protocol_coords(px, py, display_width, display_height)

    '''