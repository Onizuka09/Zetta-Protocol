# example.py
import time
import struct
from zetta_protocol import ZettaProtocol, ZettaPacketType, create_struct_parser, create_struct_builder
import sys
def main():
    # 1. Create Zetta protocol instance
    zetta = ZettaProtocol(
        port="/dev/ttyACM0",
        baudrate=115200,
        timeout=0.1,
        rx_callback=lambda packet: print(f"Callback: Received {packet.type.name} packet")
    )

    # 2. Register packet handlers

    # Handler for structured data (int, float, 5-byte string)
    zetta.register_packet_handler(
        packet_type=ZettaPacketType.MSG_PUBLISH,
        parser=create_struct_parser('<4sif'),  # Parse as (int, float, bytes)
        builder=create_struct_builder('<4sif')  # Build from (int, float, bytes)
    )

    # Handler for simple strings
    zetta.register_packet_handler(
        packet_type=ZettaPacketType.MSG_ACK,
        parser=lambda data: data.decode('utf-8'),
        builder=lambda data: data.encode('utf-8')
    )

    # 3. Start the protocol
    zetta.start()

    try:
        print("Zetta Protocol Example")
        print("Press Ctrl+C to exit")

        # 4. Send some packets

        # Send structured data
        print("\nSending structured data...")
        structured_data = (42, 99.99, b"hello")
        if zetta.send(ZettaPacketType.MSG_PUBLISH, structured_data):
            print("Structured data sent successfully")

        # Send string data
        print("\nSending string data...")
        if zetta.send_raw(ZettaPacketType.MSG_ACK, b"Hello from PC!"):
            print("String data sent successfully")

        # 5. Process received packets
        print("\nProcessing received packets...")
        run = True
        try:

            while(run):
                # for _ in range(5):  # Check for packets for 5 seconds
                packet = zetta.get_packet(timeout=0)
                if packet:
                    print(f"Received: {packet.type.name}, Size: {len(packet.data)}")

                    # Process using registered parser
                    parsed_data = zetta.process_packet(packet)
                    print(f"Parsed: {parsed_data}")
                else:
                    print("No packets received")
                time.sleep(1)

        except KeyboardInterrupt:
            print("\nInterrupted by user")
            # sys.ex
            run = False
        # You can also send more data
        # if _ == 2:
        #     print("\nSending another packet...")
        #     zetta.send_raw(ZettaPacketType.MSG_PUBLISH, b"Test\x00\x01\x02")

        # 6. Show statistics

    finally:
        # 7. Cleanup            stats = zetta.get_stats()
        stats = zetta.get_stats()
        print("\n=== Statistics ===")
        for key, value in stats.items():
            print(f"{key}: {value}")
        zetta.stop()

if __name__ == "__main__":
    main()
