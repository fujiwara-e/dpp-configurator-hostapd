#!/bin/bash

# DPP認証環境セットアップスクリプト

echo "=== DPP認証環境セットアップ ==="

# 1. 必要なパッケージの確認
echo "1. 必要なパッケージの確認..."
if ! command -v hostapd &> /dev/null; then
    echo "hostapd is not installed. Installing..."
    sudo apt update
    sudo apt install -y hostapd
else
    echo "✓ hostapd is already installed"
fi

if ! command -v iw &> /dev/null; then
    echo "iw is not installed. Installing..."
    sudo apt install -y iw
else
    echo "✓ iw is already installed"
fi

# 2. 無線インターフェースの確認
echo "2. 無線インターフェースの確認..."
echo "Available wireless interfaces:"
iw dev | grep Interface | awk '{print $2}'

# 3. hostapdのDPPサポート確認
echo "3. hostapdのDPPサポート確認..."
if hostapd -h 2>&1 | grep -q "DPP"; then
    echo "✓ hostapd supports DPP"
else
    echo "⚠ hostapd may not support DPP. You may need a newer version."
fi

# 4. サンプル設定ファイルの作成
echo "4. サンプルhostapd設定ファイルの作成..."
cat > hostapd_dpp.conf << 'EOF'
# hostapd configuration for DPP testing
interface=wlan0
driver=nl80211
ssid=DPP-Test-AP
hw_mode=g
channel=1
wmm_enabled=1
country_code=JP
ieee80211d=1
ieee80211h=1

# Enable DPP
dpp=1
dpp_configurator_params=curve=prime256v1
dpp_connector_privacy_default=1

# Control interface for DPP CLI communication
ctrl_interface=/var/run/hostapd
ctrl_interface_group=wheel

# Basic security
wpa=2
wpa_key_mgmt=WPA-PSK DPP
wpa_pairwise=TKIP CCMP
rsn_pairwise=CCMP
wpa_passphrase=testpassword

# Logging
logger_syslog=0
logger_stdout=1
logger_stdout_level=2
EOF

echo "✓ Sample hostapd configuration created: hostapd_dpp.conf"

# 5. 実行手順の表示
echo ""
echo "=== 実際のDPP認証を行う手順 ==="
echo ""
echo "1. 無線インターフェースをAP モードに設定:"
echo "   sudo iw dev wlan0 set type __ap"
echo ""
echo "2. hostapdをDPP設定で起動:"
echo "   sudo hostapd hostapd_dpp.conf"
echo ""
echo "3. 別のターミナルでDPP Configuratorを実行:"
echo "   ./dpp-configurator-hostapd auth_init_real interface=wlan0 ..."
echo ""
echo "4. またはhostapd_cliでDPPコマンドを直接実行:"
echo "   sudo hostapd_cli -i wlan0 dpp_configurator_add"
echo "   sudo hostapd_cli -i wlan0 dpp_qr_code '<QR_URI>'"
echo "   sudo hostapd_cli -i wlan0 dpp_auth_init peer=1 configurator=1 conf=sta-psk"
echo ""
echo "注意事項:"
echo "- 実際の無線通信には相手側デバイス（DPP対応端末）が必要です"
echo "- 無線インターフェースが他のプロセスで使用中でないことを確認してください"
echo "- sudo権限が必要です"
echo ""
echo "テスト用のスタンドアローン実行:"
echo "- ./dpp-configurator-hostapd auth_init peer=1 configurator=1 conf=sta-psk"
echo "  (これはhostapd通信なしのシミュレーション)"
