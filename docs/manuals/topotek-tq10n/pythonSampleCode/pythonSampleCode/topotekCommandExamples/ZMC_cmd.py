from topotekcmdparse import *
'''
"U — Uart", "M — Lens", "D — System and Image", "E — Sub-device", "P — Network", "G — Gimbal"
address_bit1 is source address, if send the command over the network, use 'P', if send the command through  Serial port, use 'U' 
'''
cmd_output = build_command(
        frame_header='#TP',
        address_bit1='U',      # source address: 
        address_bit2='M',      # destination address: System/Image module
        control_bit='w',       # write command
        identifier_bit='ZMC',  # 3-character identifier for  "ZMC"
        data='01',             # 00: stop  01: zoom out  02: zoom in
        data_mode='ASCII',     # input mode: ASCII; for Hex mode you'd provide '3031' （output_space_separate=False) or '30 31' （output_space_separate=True）
        output_format='ASCII',   # output encoded as hexadecimal string
        input_space_separate = False,
        output_space_separate=True  # insert spaces between output bytes
    )
print('ZMC ASCII =', cmd_output)