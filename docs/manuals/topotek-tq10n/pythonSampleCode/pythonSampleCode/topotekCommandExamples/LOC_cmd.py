from topotekcmdparse import *
'''
"U — Uart", "M — Lens", "D — System and Image", "E — Sub-device", "P — Network", "G — Gimbal"
address_bit1 is source address, if send the command over the network, use 'P', if send the command through  Serial port, use 'U' 
'''
def converToCommandParaValue(preview_x,
                             preview_y,
                             preview_width, 
                             preview_height, 
                             preview_trackObjectWidth, 
                             preview_trackObjectHeight,
                             blurClick_enabled = True,
                             input_space_separate = True
                             ):
    paraValue_x = round(2000*preview_x/float(preview_width)-1000)
    paraValue_y =  round(2000*preview_y/float(preview_height)-1000)
    paraValue_width = round(2000*preview_trackObjectWidth/float(preview_width))
    paraValue_height = round(2000*preview_trackObjectHeight/float(preview_height))
    print('preview_x, preview_y, preview_width,  preview_height,  preview_trackObjectWidth,  preview_trackObjectHeight', 
            preview_x,
            preview_y,
            preview_width, 
            preview_height, 
            preview_trackObjectWidth, 
            preview_trackObjectHeight)
    print('paraValue_x, paraValue_y, paraValue_width, paraValue_height = ', paraValue_x, paraValue_y, paraValue_width, paraValue_height)
 
    if blurClick_enabled:
        blurClick_value = 8
    else:
        blurClick_value = 0
    vals = (paraValue_x, paraValue_y, 
            paraValue_width, paraValue_height, 
            blurClick_value)  
            
    packed = b''.join(struct.pack('>h', v) for v in vals)
    if input_space_separate:
        return ' '.join(f'{byte:02X}' for byte in packed)
    else:
        return packed.hex().upper()

def build_loc_command(
        preview_x,
        preview_y,
        preview_width,  #only support 1920*1080 now
        preview_height, #only support 1920*1080 now
        preview_trackObjectWidth,
        preview_trackObjectHeight,
        address1,
        address2):
    input_space_separate = True
    input_data_hex = converToCommandParaValue(preview_x,
                                preview_y,
                                preview_width, 
                                preview_height, 
                                preview_trackObjectWidth, 
                                preview_trackObjectHeight,
                                input_space_separate
                                )
    print('input_data_hex = ', input_data_hex)
    cmd_hex = build_command(
    frame_header           = '#tp',                             # variable-length command
    address_bit1           = address1,                               # source address: UART. If sending from a ground station over the network, use 'P' instead.
    address_bit2           = address2,                               # destination address: Gimbal
    control_bit            = 'w',                               # read command
    identifier_bit         = 'LOC',                             # 3-character identifier
    data                   = input_data_hex,   # hex data (space-separated)
    data_mode              = 'Hex',                             # input mode: Hexadecimal
    input_space_separate   = input_space_separate,                              # input bytes must be space-separated
    output_format          = 'Hex',                             # output mode: Hexadecimal
    output_space_separate  = True                               # output bytes will be space-separated
    )
    
    return cmd_hex


if __name__=='__main__':
    ###########  # LOC command 
    loc_cmd = build_loc_command(
        preview_x   =  960,
        preview_y   = 540,
        preview_width = 1920,  #only support 1920*1080 now
        preview_height = 1080, #only support 1920*1080 now
        preview_trackObjectWidth = 64,
        preview_trackObjectHeight = 64,
        address1  = 'P',
        address2  = 'D'
    )
    print(' LOC Command = ', loc_cmd) 