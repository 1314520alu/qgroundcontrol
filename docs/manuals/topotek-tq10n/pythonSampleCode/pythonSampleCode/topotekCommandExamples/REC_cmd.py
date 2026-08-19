from topotekcmdparse import *
'''
"U — Uart", "M — Lens", "D — System and Image", "E — Sub-device", "P — Network", "G — Gimbal"
address_bit1 is source address, if send the command over the network, use 'P', if send the command through  Serial port, use 'U' 
'''
if __name__ == '__main__': 
    cmd_output = build_command(
        frame_header='#TP',
        address_bit1='U',      # source address: UART. If sending from a ground station over the network, use 'P' instead.
        address_bit2='D',      # destination address: System/Image module
        control_bit='w',       # write command
        identifier_bit='REC',  # 3-character identifier for  "Record"
        data='0A',             # payload: 00 stop record, 01 start record, 0A reverse opration
        data_mode='ASCII',     # input mode: ASCII; for Hex mode you'd provide '3031' （output_space_separate=False) or '30 31' （output_space_separate=True）
        output_format='ASCII',   # output encoded as hexadecimal string
        input_space_separate = False,
        output_space_separate=True  # insert spaces between output bytes
    )
    print(' REC ASCII =', cmd_output)