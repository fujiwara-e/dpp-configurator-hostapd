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
```bash
# Build hostapd integrated version
make

# Check dependencies
make check-hostapd

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
   - Use configurator to do provisioning :
     ```bash
     $ ./dpp-configurator-hostapd auth_init peer=1 configurator=1 conf=sta-psk interface=<network-interface> ssid=TestNetwork pass=test123
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

# License

This project integrates with hostapd/wpa_supplicant components. For licensing information regarding hostapd components, please refer to `thirdparth/LICENCE`.