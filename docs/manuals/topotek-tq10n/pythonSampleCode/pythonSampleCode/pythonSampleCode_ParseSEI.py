import threading
import subprocess
import struct
import cv2
import time


'''
SEIData in C code-------------------

typedef struct {
    double plane_lon;// degree
    double plane_lat;// degree
    float  plane_alt;// m

    uint16_t gps_year;
    uint8_t  gps_month;
    uint8_t  gps_day;
    uint8_t  gps_hour;
    uint8_t  gps_minute;
    float    gps_second;

    int32_t gimbal_yaw; //0.01 degree
    int32_t gimbal_roll; //0.01 degree
    int32_t gimbal_pitch; //0.01 degree

    double target_lon;// degree
    double target_lat;// degree
    int16_t target_x;
	int16_t target_y;
    int16_t target_size;
    int distance; //0.1m
    uint16_t highTemperature_x;
	uint16_t highTemperature_y;
	uint16_t highTemperature; //0.1 degree
	uint16_t lowTemperature_x;
	uint16_t lowTemperature_y;
	uint16_t lowTemperature; //0.1 degree
	uint16_t centerTemperature;
    int opticalZoom;
	int digitalZoom;

    uint32_t frame_id;
} SEIData;

'''

sei_struct_format = (
    '<ddfHBBBBfiii ddhhh'
    'iHHHHHHHIII'
)
sei_fields = [
    "plane_lon", "plane_lat", "plane_alt",
    "gps_year", "gps_month", "gps_day", "gps_hour", "gps_minute", "gps_second",
    "gimbal_yaw", "gimbal_roll", "gimbal_pitch",
    "target_lon", "target_lat",
    "target_x", "target_y", "target_size",
    "distance",
    "highTemperature_x", "highTemperature_y", "highTemperature",
    "lowTemperature_x", "lowTemperature_y", "lowTemperature",
    "centerTemperature",
    "opticalZoom", "digitalZoom",
    "frame_id"
]
expected_struct_size = struct.calcsize(sei_struct_format)

# UUID 
sei_uuid = bytes([
    0xA1, 0xB2, 0xC3, 0xD4, 0xE5, 0xF6, 0xA7, 0xB8,
    0xC9, 0xDA, 0xEB, 0xFC, 0x1A, 0x2B, 0x3C, 0x4D
])

 
latest_sei = {}
sei_lock = threading.Lock()

def parse_sei_payload(sei_payload: bytes): 
    if len(sei_payload) < expected_struct_size:
        return None
    unpacked = struct.unpack(sei_struct_format, sei_payload[:expected_struct_size])
    return dict(zip(sei_fields, unpacked))

def parse_sei_nal(payload: bytes): 
    i = 0
    payload_type = 0
    while i < len(payload) and payload[i] == 0xFF:
        payload_type += 0xFF
        i += 1
    if i < len(payload):
        payload_type += payload[i]
        i += 1

    payload_size = 0
    while i < len(payload) and payload[i] == 0xFF:
        payload_size += 0xFF
        i += 1
    if i < len(payload):
        payload_size += payload[i]
        i += 1

    if payload_type != 5:
        return None

    if i + 16 > len(payload):
        return None
    uuid = payload[i:i+16]
    i += 16

    if uuid != sei_uuid:
        return None

    sei_payload = payload[i:]
    return sei_payload

def sei_reader(rtsp_url, codec="h265"): 
    if codec == "h264":
        bsf_filter = "h264_mp4toannexb"
        output_fmt = "h264"
        sei_nalu_types = [6]
    elif codec == "h265":
        bsf_filter = "hevc_mp4toannexb"
        output_fmt = "hevc"
        sei_nalu_types = [39, 40]
    else:
        raise ValueError(f"Unsupported codec: {codec}")

    cmd = [
        "ffmpeg", "-rtsp_transport", "udp", "-i", rtsp_url,
        "-an", "-c:v", "copy",
        "-bsf:v", bsf_filter,
        "-f", output_fmt, "-"
    ]

    proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.DEVNULL)
    buf = b""
    try:
        while True:
            chunk = proc.stdout.read(4096)
            if not chunk:
                break
            buf += chunk
            while True:
                i = buf.find(b"\x00\x00\x00\x01")
                if i < 0:
                    break
                j = buf.find(b"\x00\x00\x00\x01", i+4)
                if j < 0:
                    break
                nal = buf[i+4:j]
                buf = buf[j:]

                if len(nal) < 3:
                    continue

                if codec == "h264":
                    nal_type = nal[0] & 0x1F
                    payload = nal[1:]
                else:
                    nal_type = (nal[0] >> 1) & 0x3F
                    payload = nal[2:]

                if nal_type in sei_nalu_types:
                    sei_payload = parse_sei_nal(payload)
                    if sei_payload:
                        data = parse_sei_payload(sei_payload)
                        if data:
                            with sei_lock:
                                latest_sei.clear()
                                latest_sei.update(data)
    finally:
        proc.kill()

def main():
    rtsp_url = "rtsp://192.168.144.108:554"  # your RTSP address
    codec = "h264"  # "h264" or "h265"  

    t = threading.Thread(target=sei_reader, args=(rtsp_url, codec), daemon=True)
    t.start()

    # play video and display SEI data
    cap = cv2.VideoCapture(rtsp_url, cv2.CAP_FFMPEG)
    if not cap.isOpened():
        print("can not open the RTSP stream")
        return

    while True:
        ret, frame = cap.read()
        if not ret:
            break

        frame = frame.copy()

        with sei_lock:
            y = 30
            for key, val in latest_sei.items():
                text = f"{key}: {val}"
                cv2.putText(frame, text, (10, y), cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 255, 0), 2)
                y += 25

        cv2.imshow("RTSP + SEI Overlay", frame)

        if cv2.waitKey(1) & 0xFF == ord('q'):
            break

    cap.release()
    cv2.destroyAllWindows()

if __name__ == "__main__":
    main()
