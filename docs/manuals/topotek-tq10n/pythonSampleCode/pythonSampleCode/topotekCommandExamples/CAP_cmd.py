from topotekcmdparse import *

'''
"U — Uart", "M — Lens", "D — System and Image", "E — Sub-device", "P — Network", "G — Gimbal"
address_bit1 is source address, if send the command over the network, use 'P', if send the command through  Serial port, use 'U' 
'''

if __name__ == '__main__': 
    ###########  1. # capture a picture ASCII data input
    
    cmd_output = build_command(
        frame_header='#TP',
        address_bit1='U',      # source address: UART. If sending from a ground station over the network, use 'P' instead.
        address_bit2='D',      # destination address: System/Image module
        control_bit='w',       # write command
        identifier_bit='CAP',  # 3-character identifier for  "Capture"
        data='01',             # payload: ASCII '01' means take one picture
        data_mode='ASCII',     # input mode: ASCII; for Hex mode you'd provide '3031' （output_space_separate=False) or '30 31' （output_space_separate=True）
        output_format='Hex',   # output encoded as hexadecimal string
        input_space_separate = False,
        output_space_separate=True  # insert spaces between output bytes
    )
    print('1 CAP Hex =', cmd_output)

    ###########  2. # capture a picture Hex data input 
    cmd_output = build_command(
        frame_header='#TP',
        address_bit1='U',      # source address: UART. If sending from a ground station over the network, use 'P' instead.
        address_bit2='D',      # destination address: System/Image module
        control_bit='w',       # write command
        identifier_bit='CAP',  # 3-character identifier for \"Capture\"
        data='3031',             # payload: ASCII '01' means take one picture
        input_space_separate = False,
        data_mode='Hex',     # input mode:  for Hex mode you'd provide '3031' （output_space_separate=False) or '30 31' （output_space_separate=True）
        output_format='Hex',   # output encoded as hexadecimal string
        output_space_separate=False  # insert spaces between output bytes
    )
    print('2 CAP Hex =', cmd_output)