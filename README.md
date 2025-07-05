# DPP Configurator

A collection of programs created to implement a configurator for Wi-Fi Easy Connect using DPP (Device Provisioning Protocol). dpp-configurator advertises using bootstrap information of device joining the network. Bootstrap information includes the Wi-Fi channel to be used and the public bootstrap key. Originally, bootstrap information is published as a QR code or NFC tag, but in this implementation, decoded information contained in the QR Code is used.

# Installation
1. Clone code
   ```bash
   $ git clone https://github.com/your-username/dpp-configurator.git
   ```

# Preparation
1. Create `credential.json` file
   
   Set ssid and password for access point
   ```json
   {"wi-fi_tech":"infra","discovery":{"ssid":"<SSID>"},"cred":{"akm":"psk","pass":"<PASSWORD>"}}
   ```
   
   Example:
   ```json
   {"wi-fi_tech":"infra","discovery":{"ssid":"TestNetwork"},"cred":{"akm":"psk","pass":"test123"}}
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

## Setup and Run

1. Setup Network Interface
   ```bash
   $ sudo ./setup_dpp_environment.sh
   ```

2. Start hostapd (in separate terminal)
   ```bash
   $ sudo /path/to/your/hostap/hostapd/hostapd hostapd_dpp.conf
   ```

3. Compile dpp-configurator
   ```bash
   $ make
   ```

4. Run dpp-configurator

   - Basic DPP functionality test:
     ```bash
     $ ./dpp-configurator-hostapd configurator_add curve=prime256v1
     ```

   - Real DPP authentication:
     ```bash
     $ ./dpp-configurator-hostapd auth_init_real interface=wlo1 peer_uri="DPP:C:81/6;M:12:34:56:78:90:ab;K:MDkwEwYH...6DjUD8=;;" ssid=TestNetwork pass=test123
     ```

5. Restore the Environment
   ```bash
   $ ./finish.sh <NI_NAME>
   ```

# Supported Commands

| Command                  | Status    | Description                             |
| ------------------------ | --------- | --------------------------------------- |
| `help`                   | ✅ Working | Display help information                |
| `status`                 | ✅ Working | Show current status                     |
| `configurator_add`       | ✅ Working | Add DPP Configurator                    |
| `dpp_qr_code`            | ✅ Working | Parse QR code                           |
| `bootstrap_get_uri`      | ✅ Working | Get bootstrap information               |
| `auth_init_real`         | ✅ Working | Start real wireless DPP authentication  |
| `auth_status`            | ✅ Working | Show authentication status              |
| `auth_monitor`           | ✅ Working | Monitor DPP authentication events       |
| `auth_control`           | ✅ Working | Control DPP authentication (start/stop) |
| `config_request_monitor` | ✅ Working | Monitor Configuration Request           |