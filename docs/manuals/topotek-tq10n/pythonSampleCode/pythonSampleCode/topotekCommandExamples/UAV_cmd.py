from topotekcmdparse import *

'''
"U — Uart", "M — Lens", "D — System and Image", "E — Sub-device", "P — Network", "G — Gimbal"
address_bit1 is source address, if send the command over the network, use 'P', if send the command through  Serial port, use 'U' 
'''


def build_uav_command(
    azimuth_deg: float,
    pitch_deg: float,
    roll_deg: float,
    speed_ms: float,
    path_angle_deg: float,
    address1: str = 'U',
    address2: str = 'A'
) -> str:
    """
    Build a UAV command.

    Parameters:
      azimuth_deg:    Yaw (azimuth) angle in degrees (°). Scaled by 100 and stored as int16 (signed).
      pitch_deg:      Pitch angle in degrees. Scaled by 100 and stored as int16.
      roll_deg:       Roll angle in degrees. Scaled by 100 and stored as int16 (signed).
      speed_ms:       Speed in m/s. Scaled by 100 and stored as uint16.
      path_angle_deg: Flight path angle in degrees. Scaled by 100 and stored as uint16.
      address1:       Source address character (default 'U').
      address2:       Destination address character (default 'A').

    Returns:
      Complete Hex command string with spaces.
    """
    # Scale values
    azi_val  = int(round(azimuth_deg    * 100))
    pit_val  = int(round(pitch_deg      * 100))
    rol_val  = int(round(roll_deg       * 100))
    spd_val  = int(round(speed_ms       * 100))
    path_val = int(round(path_angle_deg * 100))

    # Pack fields in little-endian order:
    # h = int16, H = uint16
    data_bytes = struct.pack('<hhhHH', azi_val, pit_val, rol_val, spd_val, path_val)

    # Convert to space-separated hex string
    data_hex = ' '.join(f'{b:02X}' for b in data_bytes)

    return build_command(
        frame_header='#tp',
        address_bit1=address1,
        address_bit2=address2,
        control_bit='w',
        identifier_bit='UAV',
        data=data_hex,
        data_mode='Hex',
        input_space_separate=True,
        output_format='Hex',
        output_space_separate=True
    )


if __name__=='__main__':
    ###########  # UAV command  # Example: yaw -120.30°, pitch +15.20°, roll -30.80°, speed 88.90 m/s, path angle 100.00°  
    uav_cmd = build_uav_command(
        azimuth_deg     = -120.30,
        pitch_deg       =   15.20,
        roll_deg        =  -30.80,
        speed_ms        =   88.90,
        path_angle_deg  =  100.00,
        address1        = 'P',
        address2        = 'D'
    )
    print('  UAV Command:', uav_cmd)