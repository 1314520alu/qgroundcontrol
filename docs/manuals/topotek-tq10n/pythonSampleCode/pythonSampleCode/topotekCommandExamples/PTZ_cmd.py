from topotekcmdparse import *
'''
"U — Uart", "M — Lens", "D — System and Image", "E — Sub-device", "P — Network", "G — Gimbal"
address_bit1 is source address, if send the command over the network, use 'P', if send the command through  Serial port, use 'U' 
''' 

cmd_output = build_command(
        frame_header='#TP',
        address_bit1='U',      # source address: UART. If sending from a ground station over the network, use 'P' instead.
        address_bit2='G',      # destination address: Gimbal
        control_bit='w',       # write command
        identifier_bit='PTZ',  # 3-character identifier for  "PTZ"
        data='01',             # stop: 00  up: 01  down: 02  left: 03  right: 04  return home: 05
        data_mode='ASCII',     # input mode: ASCII; for Hex mode you'd provide '3031' （output_space_separate=False) or '30 31' （output_space_separate=True）
        output_format='Hex',   # output encoded as hexadecimal string
        input_space_separate = False,
        output_space_separate=True  # insert spaces between output bytes
    )
print('PTZ Hex =', cmd_output)



cmd_output = build_command(
        frame_header='#TP',
        address_bit1='U',      # source address: UART. If sending from a ground station over the network, use 'P' instead.
        address_bit2='G',      # destination address: Gimbal
        control_bit='w',       # write command
        identifier_bit='PTZ',  # 3-character identifier for  "PTZ"
        data='01',             # stop: 00  up: 01  down: 02  left: 03  right: 04  return home: 05
        data_mode='ASCII',     # input mode: ASCII; for Hex mode you'd provide '3031' （output_space_separate=False) or '30 31' （output_space_separate=True）
        output_format='ASCII',   # output encoded as hexadecimal string
        input_space_separate = False,
        output_space_separate=True  # insert spaces between output bytes
    )
print('PTZ ASCII =', cmd_output)