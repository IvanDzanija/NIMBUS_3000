# WhatsDown Mobile

Flutter companion app for the parent side of WhatsDown. It connects to the MQTT
broker, subscribes to messages from the ESP32 kid board, sends replies, and keeps
a small local message history with `shared_preferences`.

## Run

```bash
flutter pub get
flutter run --dart-define=BROKER_IP=192.168.1.10 --dart-define=MY_ID=Mom
```

Useful runtime values:

- `BROKER_IP`: IP address of the machine running Mosquitto.
- `MY_ID`: parent identity, usually `Mom` or `Dad`.
- `KID_ID`: kid board identity, defaults to `kid`.

## Files

- `lib/main.dart` builds the chat UI and handles connection states.
- `lib/mqtt_service.dart` wraps MQTT connect, subscribe, publish, and errors.
- `lib/message_store.dart` stores recent messages locally.

The platform folders (`android/`, `ios/`, `web/`, `macos/`, `linux/`,
`windows/`) are the normal Flutter generated host projects.
