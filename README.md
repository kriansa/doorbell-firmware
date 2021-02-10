# My doorbell firmware

## Overview

[I'm thinking about my doorbell](https://www.youtube.com/watch?v=IlcMRq3gb1s). When you're gonna
ring it?

This is an app that uses a ESP-01 to capture a doorbell button and optionally power a electric door
lock.

It works by connecting to a MQTT broker to communicate with the outside applications, so it can use
AWS IOT as well.

## Setup

- Install [mos tool](https://mongoose-os.com/software.html)
- Copy `fs/conf2.json` to `fs/conf3.json` and change your settings accordingly
- Change `.env` if needed
- Run `make deploy`

**Important**: Your secrets will be stored on `fs/conf3.json`. Beware not to
leak them.

## Configuring MQTT

After copying `fs/conf2.json` to `fs/conf3.json`, you will need to set the
right parameters to connect to your MQTT server. Please refer to
[Mos MQTT Docs](https://mongoose-os.com/docs/mongoose-os/api/net/mqtt.md) to
see all available parameters.

### For AWS IoT

You will need to set your `fs/conf3.json` configuration as such:

```json
{
  "mqtt": {
    "ssl_ca_cert": "ca.pem",
    "ssl_cert": "cert.pem",
    "ssl_key": "key.pem"
  }
}
```

You may wonder where does `ca.pem` come from. It's coming from `ca-bundle` pkg.

### Configurable properties

All properties below can be configured remotely through MQTT. To do so, you just need to:

```sh
export MOS_PORT=mqtt://mqtt.eclipse.org/my-device-name
mos config-set my.config=true
```

#### Available properties

* `app.console` (default: true) - Enable UART (debugging through USB port). This is useful for debugging purposes
  while you have the ESP device connected to the computer. However, whenever you need to actually
  deploy it, you will want to turn it off. That's because the console uses the same GPIO port as the
  on-board blue LED on ESP-01, meaning you can't use both. We use the blue led for visually sign if
  there's something wrong with the device.
* `app.relay_time` (default: 500) - How long should be the relay pulse to open the door.
* `app.mqtt.button_topic` (default: btn-press) - The MQTT topic to send button press events to. It's
  automatically prepended by `devices/<device_id>/`.
* `app.mqtt.relay_topic` (default: cmd-relay) - The MQTT topic to subscribe for relay events. It's
  automatically prepended by `devices/<device_id>/`.

## License

The contents of this repository is licensed under the [Apache v2 License](LICENSE).
