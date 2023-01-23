# High Level Analyzer
# For more information and documentation, please go to https://support.saleae.com/extensions/high-level-analyzer-extensions

from enum import Enum, unique
from saleae.analyzers import HighLevelAnalyzer, AnalyzerFrame, StringSetting, NumberSetting, ChoicesSetting
from saleae.data import GraphTime, GraphTimeDelta


@unique
class PacketType(str, Enum):
    status_update = "Status Update"
    power_control = "Power Control"
    data_request  = "Data Request"
    simple_query  = "Simple Query"
    multiple      = "Multiple"


@unique
class Packet(Enum):
    signal_strength         = (0x01, 1, 'SIG', 'Signal Strength', PacketType.status_update)
    end_power_transfer      = (0x02, 1, 'EPT', 'End Power Transfer', PacketType.power_control)
    control_error           = (0x03, 1, 'CE', 'Control Error', PacketType.power_control)
    received_power_8b       = (0x04, 1, 'RP8', 'Received Power (8 bit)', PacketType.status_update)
    charge_status           = (0x05, 1, 'CHS', 'Charge Status', PacketType.status_update)
    power_control_hold_off  = (0x06, 1, 'PCH', 'Power Control Hold-Off', PacketType.status_update)
    general_request         = (0x07, 1, 'GRQ', 'General Request', PacketType.data_request)
    renegotiate             = (0x09, 1, 'NEGO', 'Renegotiate', PacketType.simple_query)
    data_stream_response    = (0x15, 1, 'DSR', 'Data Stream Response', PacketType.data_request)
    aux_data_transport_1e   = (0x16, 1, 'ADT/1e', 'Aux Data Transport (even)', PacketType.simple_query)
    aux_data_transport_1o   = (0x17, 1, 'ADT/1o', 'Aux Data Transport (odd)', PacketType.simple_query)
    proprietary_1e          = (0x18, 1, 'PROP/1e', 'Proprietary', PacketType.multiple)
    proprietary_1o          = (0x19, 1, 'PROP/1o', 'Proprietary', PacketType.multiple)
    specific_request        = (0x20, 2, 'SRQ', 'Specific Request', PacketType.simple_query)
    fod_status              = (0x22, 2, 'FOD', 'FOD Status', PacketType.simple_query)
    aux_data_control        = (0x25, 2, 'ADC', 'Aux Data Control', PacketType.simple_query)
    aux_data_transport_2e   = (0x26, 2, 'ADT/2e', 'Aux Data Transport (even)', PacketType.simple_query)
    aux_data_transport_2o   = (0x27, 2, 'ADT/2o', 'Aux Data Transport (odd)', PacketType.simple_query)
    proprietary_2e          = (0x28, 2, 'PROP/2e', 'Proprietary', PacketType.multiple)
    proprietary_2o          = (0x29, 2, 'PROP/2o', 'Proprietary', PacketType.multiple)
    received_power_16b      = (0x31, 3, 'RP', 'Received Power (16 bit)', PacketType.simple_query)
    aux_data_transport_3e   = (0x36, 3, 'ADT/3e', 'Aux Data Transport (even)', PacketType.simple_query)
    aux_data_transport_3o   = (0x37, 3, 'ADT/3o', 'Aux Data Transport (odd)', PacketType.simple_query)
    proprietary_3           = (0x38, 3, 'PROP/3', 'Proprietary', PacketType.multiple)
    aux_data_transport_4e   = (0x46, 4, 'ADT/4e', 'Aux Data Transport (even)', PacketType.simple_query)
    aux_data_transport_4o   = (0x47, 4, 'ADT/4o', 'Aux Data Transport (odd)', PacketType.simple_query)
    proprietary_4           = (0x48, 4, 'PROP/4', 'Proprietary', PacketType.multiple)
    configuration           = (0x51, 5, 'CFG', 'Configuration', PacketType.simple_query)
    wireless_power_id_msb   = (0x54, 5, 'WPID/msb', 'Wireless Power ID (msb)', PacketType.simple_query)
    wireless_power_id_lsb   = (0x55, 5, 'WPID/lsb', 'Wireless Power ID (lsb)', PacketType.simple_query)
    aux_data_transport_5e   = (0x56, 5, 'ADT/5e', 'Aux Data Transport (even)', PacketType.simple_query)
    aux_data_transport_5o   = (0x57, 5, 'ADT/5o', 'Aux Data Transport (odd)', PacketType.simple_query)
    proprietary_5           = (0x58, 5, 'PROP/5', 'Proprietary', PacketType.multiple)
    aux_data_transport_6e   = (0x66, 6, 'ADT/6e', 'Aux Data Transport (even)', PacketType.simple_query)
    aux_data_transport_6o   = (0x67, 6, 'ADT/6o', 'Aux Data Transport (odd)', PacketType.simple_query)
    proprietary_6           = (0x68, 6, 'PROP/6', 'Proprietary', PacketType.multiple)
    identification          = (0x71, 7, 'ID', 'Identification', PacketType.status_update)
    aux_data_transport_7e   = (0x76, 7, 'ADT/7e', 'Aux Data Transport (even)', PacketType.simple_query)
    aux_data_transport_7o   = (0x77, 7, 'ADT/7o', 'Aux Data Transport (odd)', PacketType.simple_query)
    proprietary_7           = (0x78, 7, 'PROP/7', 'Proprietary', PacketType.multiple)
    extended_identification = (0x81, 8, 'XID', 'Extended Identification', PacketType.status_update)
    proprietary_8           = (0x84, 8, 'PROP/8', 'Proprietary', PacketType.multiple)
    proprietary_12          = (0xA4, 12, 'PROP/12', 'Proprietary', PacketType.multiple)
    proprietary_16          = (0xC4, 16, 'PROP/16', 'Proprietary', PacketType.multiple)
    proprietary_20          = (0xE4, 20, 'PROP/20', 'Proprietary', PacketType.multiple)

    def __new__(cls, header, size, mnemonic, full_name, packet_type):
        entry = object.__new__(cls)
        entry._value_ = header

        entry.header      = header
        entry.size        = size
        entry.mnemonic    = mnemonic
        entry.full_name   = full_name
        entry.packet_type = packet_type
        return entry

    def __str__(self):
        # return f'<{type(self).__name__}.{self.name}: ({self.a!r}, {self.b!r})>'
        header_hex = "0x{:02x}".format(self.header)
        return f'{self.mnemonic} ({header_hex}): {self.full_name}'


@unique
class ByteType(Enum):
    unknown  = 0
    header   = 1
    message  = 2
    checksum = 3


# High level analyzers must subclass the HighLevelAnalyzer class.
class Hla(HighLevelAnalyzer):
    # List of settings that a user can set for this High Level Analyzer.
    # my_string_setting = StringSetting()
    # my_number_setting = NumberSetting(min_value=0, max_value=100)
    # my_choices_setting = ChoicesSetting(choices=('A', 'B'))

    # An optional list of types this analyzer produces, providing a way to customize the way frames are displayed in Logic 2.
    # 'format': 'Output type: {{type}}, Input type: {{data.input_type}}'
    result_types = {
        'header': {
            'format': '{{data.value}}'
        },
        'message': {
            'format': '{{data.value}}'
        },
        'checksum': {
            'format': 'Checksum {{data.value}}'
        },
        'unknown': {
            'format': 'Unknown ({{data.value}})'
        }
    }

    def __init__(self):
        '''
        Initialize HLA.

        Settings can be accessed using the same name used above.
        '''

        # print("Settings:", self.my_string_setting,
        #       self.my_number_setting, self.my_choices_setting)

        self.packet = None
        self.last_packet_start_time = None
        self.last_packet_end_time = None

        self.last_packet_byte = ByteType.unknown
        self.next_packet_byte = ByteType.header
        self.last_packet_byte_start_time = None
        self.last_packet_byte_end_time = None

        self.remaining_message_bytes = 0

        self.packet_bytes = []
    
    def compute_checksum(self):
        checksum = 0
        for val in self.packet_bytes:
            checksum = checksum ^ val
        return checksum

    def decode(self, frame: AnalyzerFrame):
        '''
        Process a frame from the input analyzer, and optionally return a single `AnalyzerFrame` or a list of `AnalyzerFrame`s.

        The type and data values in `frame` will depend on the input analyzer.
        '''

        val = frame.data['data']
        packet_byte = frame.data['packet_byte']

        start_bit = (val & 0b00000000001) >> 0
        parity_bit = (val & 0b01000000000) >> 9
        stop_bit = (val & 0b10000000000) >> 10
        data_val = (val & 0b00111111110) >> 1

        display_val = "0x{:02x}".format(data_val)

        if self.last_packet_byte_end_time is not None:
            gap = frame.start_time - self.last_packet_byte_end_time
            if gap > GraphTimeDelta(millisecond=1):  # 1mS
                self.next_packet_byte = ByteType.header
                self.packet = None
                self.last_packet_end_time = self.last_packet_byte_end_time

        curr_packet_byte = self.next_packet_byte
        if curr_packet_byte == ByteType.header:
            if data_val in Packet._value2member_map_:
                self.packet = Packet(data_val)
                self.last_packet_start_time = frame.start_time
                self.last_packet_end_time = None

                self.remaining_message_bytes = self.packet.size
                self.packet_bytes = [data_val]

                display_val = str(self.packet)

                self.next_packet_byte = ByteType.message
            else:
                self.packet = None
                self.last_packet_start_time = frame.start_time
                self.last_packet_end_time = frame.end_time

                self.remaining_message_bytes = 0

                self.next_packet_byte = ByteType.header
                curr_packet_byte = ByteType.unknown
        elif curr_packet_byte == ByteType.message:
            self.packet_bytes.append(data_val)

            self.remaining_message_bytes = self.remaining_message_bytes - 1
            if self.remaining_message_bytes == 0:
                self.next_packet_byte = ByteType.checksum
        elif curr_packet_byte == ByteType.checksum:
            self.next_packet_byte = ByteType.header

            self.last_packet_end_time = frame.end_time

            checksum = self.compute_checksum()
            if checksum == data_val:
                display_val = f'OK: {display_val}'
            else:
                display_checksum = "0x{:02x}".format(checksum)
                display_val = f'INCONSISTENT: {display_val} != {display_checksum}'

        self.last_packet_byte = curr_packet_byte
        self.last_packet_byte_start_time = frame.start_time
        self.last_packet_byte_end_time = frame.end_time

        # Return the data frame itself
        return AnalyzerFrame(curr_packet_byte.name, frame.start_time, frame.end_time, {
            'value': display_val
        })
