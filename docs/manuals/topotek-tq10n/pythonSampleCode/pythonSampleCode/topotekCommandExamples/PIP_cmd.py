from topotekcmdparse import *
'''
"U — Uart", "M — Lens", "D — System and Image", "E — Sub-device", "P — Network", "G — Gimbal"
address_bit1 is source address, if send the command over the network, use 'P', if send the command through  Serial port, use 'U' 
'''

cmd_output = build_command(
        frame_header='#tp',
        address_bit1='P',      # source address: 
        address_bit2='D',      # destination address: System/Image module
        control_bit='w',       # write command
        identifier_bit='PIP',  # 3-character identifier for  "PIP"
        data='0A',             # 0A : next mode
        data_mode='ASCII',     # input mode: ASCII; for Hex mode you'd provide '3031' （output_space_separate=False) or '30 31' （output_space_separate=True）
        output_format='Hex',   # output encoded as hexadecimal string
        input_space_separate = False,
        output_space_separate=True  # insert spaces between output bytes
    )
print('PIP Hex =', cmd_output)



cmd_output = build_command(
        frame_header='#tp',
        address_bit1='P',      # source address: 
        address_bit2='D',      # destination address: System/Image module
        control_bit='w',       # write command
        identifier_bit='PIP',  # 3-character identifier for  "PIP"
        data='0A',             # 0A : next mode
        data_mode='ASCII',     # input mode: ASCII; for Hex mode you'd provide '3031' （output_space_separate=False) or '30 31' （output_space_separate=True）
        output_format='ASCII',   # output encoded as hexadecimal string
        input_space_separate = False,
        output_space_separate=True  # insert spaces between output bytes
    )
print('PIP ASCII =', cmd_output)