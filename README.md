# DPP Configurator

A collection of programs created to implement a configurator for Wi-Fi Easy Connect using DPP (Device Provisioning Protocol). dpp-configurator advertises using bootstrap information of device joining the network. Bootstrap information includes the Wi-Fi channel to be used and the public bootstrap key. Originally, bootstrap information is published as a QR code or NFC tag, but in this implementation, decoded information contained in the QR Code is used.

# Installation
1. Clone code
   ```bash
   $ git clone https://github.com/fujiwara-e/dpp-configurator-hostapd.git
   ```

# How to use

## Prerequisites
- Linux environment
- gcc, OpenSSL development libraries
- netlink development libraries
- DPP-compatible hostapd source code (https://w1.fi/cvs.html)

## Build

1. First time setup - configure Makefile:
   ```bash
   $ cp Makefile.sample Makefile
   $ vim Makefile
   # Set HOSTAPD_DIR to your hostapd source directory path
   ```

2. Build the project:
   ```bash
   # Build
   make

   # Clean up
   make clean
   ```

## Hostapd configuration

1. Copy and configure hostapd configuration file:
   ```bash
   $ cp hostapd_dpp.conf.sample hostapd_dpp.conf
   ```

2. Edit `hostapd_dpp.conf` to match your network interface:
   ```bash
   $ vim hostapd_dpp.conf
   # Change 'interface' 'channel' 'ctrl_interface' to your actual settings 
   ```

## Setup and Run

1. Compile dpp-configurator-hostapd
   ```bash
   $ make clean && make
   ```

2. Setup Network Interface and start hostapd (in separate terminal)
   ```bash
   $ sudo systemctl stop NetworkManager
   $ sudo /path/to/your/hostap/hostapd/hostapd hostapd_dpp.conf
   ```

3. Run dpp-configurator

   - Add configurator:
     ```bash
     $ ./dpp-configurator-hostapd configurator_add curve=prime256v1
     ```
   - Add enrollee (peer):
     ```bash
     $ ./dpp-configurator-hostapd dpp_qr_code "DPP:C:81/6;M:12:34:56:78:90:ab;K:MDkwEwYH...6DjUD8=;;"
     ```
   - Use configurator to do provisioning (traditional parameters):
     ```bash
     $ ./dpp-configurator-hostapd auth_init peer=1 configurator=1 conf=sta-psk interface=<network-interface> ssid=TestNetwork pass=test123
     ```

   - Use configurator with Matter PIN for IoT device provisioning:
     ```bash
     $ ./dpp-configurator-hostapd auth_init peer=1 configurator=1 conf=sta-psk interface=<network-interface> ssid=TestNetwork pass=test123 matter_pin=12345678
     ```

   - Use configurator with JSON configuration (recommended for complex setups):
     ```bash
     $ ./dpp-configurator-hostapd auth_init peer=1 configurator=1 interface=<network-interface> conf_json='{"wi-fi_tech":"infra","discovery":{"ssid":"TestNetwork"},"cred":{"akm":"psk","pass":"test123"}}'
     ```

   - Use configurator with JSON configuration including Matter:
     ```bash
     $ ./dpp-configurator-hostapd auth_init peer=1 configurator=1 interface=<network-interface> conf_json='{"wi-fi_tech":"infra","discovery":{"ssid":"TestNetwork"},"cred":{"akm":"psk","pass":"test123"},"matter":{"pinCode":"12345678","discriminator":"3840","vendorId":"65521"}}'
     ```

4. Restore the Environment
   ```bash
   $ ./finish.sh <NI_NAME>
   ```

# Supported Commands

| Command             | Description               |
| ------------------- | ------------------------- |
| `help`              | Display help information  |
| `status`            | Show current status       |
| `configurator_add`  | Add DPP Configurator      |
| `dpp_qr_code`       | Parse QR code             |
| `bootstrap_get_uri` | Get bootstrap information |
| `auth_init`         | Start DPP authentication  |

## Matter Integration

This configurator supports Matter PIN code distribution via DPP. When configuring devices that support Matter:

### Traditional Parameter Method
- Add `matter_pin=XXXXXXXX` parameter to auth_init command
- PIN must be exactly 8 digits
- Matter PIN is included in DPP configuration object sent to enrollee
- Enrollee can use the PIN for subsequent Matter commissioning

Example with Matter (traditional):
```bash
./dpp-configurator-hostapd auth_init peer=1 configurator=1 conf=sta-psk interface=wlan0 ssid=IoTNetwork pass=secret123 matter_pin=87654321
```

### JSON Configuration Method (Recommended)
- Use `conf_json` parameter with complete JSON configuration object
- Supports all Matter parameters including pinCode, discriminator, vendorId, etc.
- More flexible and extensible than traditional parameters
- JSON must be enclosed in single quotes to avoid shell interpretation

Example with Matter (JSON):
```bash
./dpp-configurator-hostapd auth_init peer=1 configurator=1 interface=wlan0 conf_json='{"wi-fi_tech":"infra","discovery":{"ssid":"IoTNetwork"},"cred":{"akm":"psk","pass":"secret123"},"matter":{"pinCode":"87654321","discriminator":"3840","vendorId":"65521","productId":"32768"}}'
```

### JSON Configuration Structure
The JSON configuration object follows the DPP specification structure:
```json
{
  "wi-fi_tech": "infra",
  "discovery": {
    "ssid": "YourNetworkName"
  },
  "cred": {
    "akm": "psk",
    "pass": "yourpassword"
  },
  "matter": {
    "pinCode": "12345678",
    "discriminator": "3840",
    "vendorId": "65521",
    "productId": "32768",
    "deviceType": "257"
  }
}
```

### Usage Notes
- Cannot mix `conf_json` with traditional parameters (conf, ssid, pass, matter_pin)
- Use either JSON configuration OR traditional parameters, not both
- Single quotes around JSON prevent shell interpretation of special characters
- JSON method is more flexible for complex configurations and future extensions

# License

This project integrates with hostapd/wpa_supplicant components. For licensing information regarding hostapd components, please refer to `thirdparth/LICENCE`.