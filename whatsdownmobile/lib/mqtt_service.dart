import 'package:mqtt_client/mqtt_client.dart';
import 'package:mqtt_client/mqtt_server_client.dart';
import 'package:flutter/foundation.dart'; 

class MqttService {
  final String brokerIp;
  final String myId;
  final String kidId = "kid";

  late MqttServerClient _client;

  Function(String message)? onMessageFromKid;
  VoidCallback? onConnected;      // ✅ added
  VoidCallback? onDisconnected;   // ✅ added
  Function(String)? onError;      // ✅ added

  MqttService({required this.brokerIp, required this.myId});

  String get _inboxTopic  => "chat/$kidId/$myId";
  String get _outboxTopic => "chat/$myId/$kidId";

  Future<void> connect() async {
    _client = MqttServerClient(brokerIp, myId)
      ..port = 1883
      ..keepAlivePeriod = 20
      ..logging(on: false)
      ..onDisconnected = () => onDisconnected?.call();

    _client.connectionMessage = MqttConnectMessage()
        .withClientIdentifier(myId)
        .withWillQos(MqttQos.atLeastOnce);

    try {
      await _client.connect();
    } catch (e) {
      onError?.call(e.toString());   // ✅ surfaces the real error in UI
      _client.disconnect();
      return;
    }

    if (_client.connectionStatus?.state == MqttConnectionState.connected) {
      onConnected?.call();           // ✅ tells the UI we're live
      _subscribe();
    } else {
      onError?.call(
        _client.connectionStatus?.returnCode.toString() ?? "unknown"
      );
    }
  }

  void _subscribe() {
    _client.subscribe(_inboxTopic, MqttQos.atLeastOnce);
    _client.updates?.listen((events) {
      for (final event in events) {
        final msg = event.payload as MqttPublishMessage;
        final payload = MqttPublishPayload.bytesToStringAsString(
          msg.payload.message,
        );
        onMessageFromKid?.call(payload);
      }
    });
  }

  void sendToKid(String message) {
    final builder = MqttClientPayloadBuilder()..addString(message);
    _client.publishMessage(_outboxTopic, MqttQos.atLeastOnce, builder.payload!);
  }

  void disconnect() => _client.disconnect();
}
