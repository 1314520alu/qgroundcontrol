from topotekcmdparse import *
'''
"U — Uart", "M — Lens", "D — System and Image", "E — Sub-device", "P — Network", "G — Gimbal"
address_bit1 is source address, if send the command over the network, use 'P', if send the command through  Serial port, use 'U' 
'''
def build_gps_command(
    yaw_deg: float,
    lon_deg: float,
    lat_deg: float,
    alt_m: float,
    address1: str = 'U',
    address2: str = 'D'
) -> str:
    """
    Build a GPS command.

    Parameters:
      yaw_deg:   Yaw angle in degrees (°). Scaled by 100 and stored as int16.
      lon_deg:   Longitude in degrees. Scaled by 1e7 and stored as uint32.
      lat_deg:   Latitude in degrees. Scaled by 1e7 and stored as uint32.
      alt_m:     Altitude in meters. Scaled by 1000 and stored as uint32.
      address1:  Source address character (default 'U').
      address2:  Destination address character (default 'D').

    Returns:
      Complete Hex command string with spaces.
    """
    # Scale values to integer representation
    yaw_val = int(round(yaw_deg * 100))
    lon_val = int(round(lon_deg * 1e7))
    lat_val = int(round(lat_deg * 1e7))
    alt_val = int(round(alt_m * 1000))

    # Pack fields in little-endian order:
    # H = uint16, I = uint32
    data_bytes = struct.pack('<HIII', yaw_val, lon_val, lat_val, alt_val)

    # Convert to space-separated hex string
    data_hex = ' '.join(f'{b:02X}' for b in data_bytes)

    # Assemble full command using build_command
    return build_command(
        frame_header='#tp',
        address_bit1=address1,
        address_bit2=address2,
        control_bit='w',
        identifier_bit='GPS',
        data=data_hex,
        data_mode='Hex',
        input_space_separate=True,
        output_format='Hex',
        output_space_separate=True
    )


if __name__=='__main__':
    ###########   # GPS command  # Example: yaw 2.70°, lon 120.20208°, lat 30.18374°, alt 0.500 m
    gps_cmd = build_gps_command(
        yaw_deg   =  2.70,
        lon_deg   = 120.20208,
        lat_deg   =  30.18374,
        alt_m     =   0.500,
        address1  = 'P',
        address2  = 'D'
    )
    print('  GPS Command = ', gps_cmd)