Bonjour Conformance Test (BCT)
==============================

## Configure Raspberry Pi for Bonjour Conformance Test

Raspberry Pi can be setup for BCT by going through the following commands:

- Compile *LightBulb* app for RaspberryPi

```sh
make TARGET=Raspi APPS=Lightbulb
```

-  Provision and install the application by running the following command:

```sh
./Tools/install.sh \
    -d raspi \
    -t ip \
    -a Output/Raspi-armv6k-unknown-linux-gnueabihf/Debug/IP/Applications/Lightbulb.OpenSSL \
    -c 5
    -n <host_name> \
    -p <password> \
    -u <user> \
    -k \
    -i
```

- Configure LightBulb as a service that is automatically restarted on power on:
    - Create a file */etc/systemd/system/lightbulb.service* with the following content:

    ```sh
    [Unit]
    Description=Acme LightBulb server on port 8080

    [Service]
    WorkingDirectory=/home/pi/acme-lightbulb/bin
    ExecStart=/home/pi/acme-lightbulb/bin/Lightbulb.OpenSSL
    Restart=on-failure

    [Install]
    WantedBy=multi-user.target
    ```

    - `sudo systemctl daemon-reload`
    - `sudo systemctl enable lightbulb.service`
    - `sudo systemctl start lightbulb.service`

- Prevent *host-local.service* from interfering with *lightbulb.service* during BCT:
    - `sudo systemctl stop host-local.service`
    - `sudo systemctl disable host-local.service`

- To allow ARP packets on the network, add line before exit 0 in file */etc/rc.local*. For example, for WiFi:
    - `ip link set wlan0 promisc on`

``` Important::
    These steps are applicable to all network interfaces supported on the accessory, ethernet or WiFi.
```

- Update file */etc/dhcpcd.conf*:
    - Replace line `persistent` with line `#persistent`

- Update file */lib/systemd/system/dhcpcd.service*:
    - Replace line `ExecStop=/sbin/dhcpcd -x` with line `ExecStop=/sbin/dhcpcd -x -k` and add the line
    `Restart=always` at end of the `[Service]` block.

- Reboot Raspberry Pi:
    - `sudo reboot`

``` Note::
    During Bonjour Conformance Test, Raspberry Pi's hostname and IP address are changed several times. Therefore the
    Raspberry Pi might not be accessible via its hostname during and after the test, and a reboot is recommended after
    the test. During reboot, the Raspberry Pi's hostname is reset to the previously supplied hostname.
```

Running LightBulb as a Linux service that automatically starts upon reboot is necessary for BCT and for a licensed
accessory. However, during development it is inconvenient. To stop and remove autostart of LightBulb as a service,
use the following commands:

```sh
sudo systemctl stop lightbulb.service
sudo systemctl disable lightbulb.service
```

When the LightBulb service is disabled, the *host-local.service* should be restarted in order to re-enable hostname
resolution; otherwise Raspberry Pi will not be accessible via its hostname and *ssh pi@raspberrypi.local* will fail.
To restart host-local.service, use the following commands:

```sh
sudo systemctl enable host-local.service
sudo systemctl start host-local.service
sudo reboot
```

## Execute the Bonjour Conformance Test

Prepare environment and execute BCT according to the Bonjour Conformance Test for HomeKit document. For the cable
change test and hot plug test during BCT, issue the following command on the Raspberry Pi. For example, for WiFi
interface *wlan0* do the following:

```sh
sudo ip link set wlan0 down && sleep 11 && sudo ip link set wlan0 up
```
