import 'package:flutter/material.dart';
import 'mqtt_service.dart';

void main() {
  runApp(ChatApp(myId: "Mom"));
}
class ChatApp extends StatelessWidget {
  final String myId;
  const ChatApp({super.key, required this.myId});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'WhatsDown',
      theme: ThemeData(colorSchemeSeed: Colors.teal),
      home: ChatScreen(myId: myId),
    );
  }
}

class ChatScreen extends StatefulWidget {
  final String myId;
  const ChatScreen({super.key, required this.myId});

  @override
  State<ChatScreen> createState() => _ChatScreenState();
}

class _ChatScreenState extends State<ChatScreen> {
  late MqttService _mqtt;
  final List<String> _messages = [];
  final TextEditingController _controller = TextEditingController();
  String _status = "Connecting...";
  bool _connected = false;

  @override
  void initState() {
    super.initState();

    _mqtt = MqttService(
      brokerIp: "172.16.55.93",
      myId: widget.myId,
    );

    _mqtt.onMessageFromKid = (msg) {
      setState(() => _messages.add("kid: $msg"));
    };

    // ✅ Listen to connection state changes
    _mqtt.onConnected = () {
      setState(() {
        _status = "Connected ✓";
        _connected = true;
      });
    };

    _mqtt.onDisconnected = () {
      setState(() {
        _status = "Disconnected ✗";
        _connected = false;
      });
    };

    _mqtt.onError = (e) {
      setState(() {
        _status = "Error: $e";
        _connected = false;
      });
    };

    _mqtt.connect();
  }

  void _send() {
    final text = _controller.text.trim();
    if (text.isEmpty || !_connected) return;
    _mqtt.sendToKid(text);
    setState(() => _messages.add("me: $text"));
    _controller.clear();
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: Text("Chat as ${widget.myId}"),
        // ✅ Shows connection state right in the app bar
        bottom: PreferredSize(
          preferredSize: const Size.fromHeight(24),
          child: Container(
            width: double.infinity,
            color: _connected ? Colors.green[700] : Colors.red[700],
            padding: const EdgeInsets.symmetric(vertical: 4),
            child: Text(
              _status,
              textAlign: TextAlign.center,
              style: const TextStyle(color: Colors.white, fontSize: 12),
            ),
          ),
        ),
      ),
      body: Column(
        children: [
          Expanded(
            child: _messages.isEmpty
                ? Center(
                    child: Text(
                      _connected
                          ? "No messages yet"
                          : "Waiting for connection...",
                      style: TextStyle(color: Colors.grey[500]),
                    ),
                  )
                : ListView.builder(
                    padding: const EdgeInsets.all(12),
                    itemCount: _messages.length,
                    itemBuilder: (_, i) => Padding(
                      padding: const EdgeInsets.symmetric(vertical: 4),
                      child: Text(_messages[i]),
                    ),
                  ),
          ),
          Padding(
            padding: const EdgeInsets.all(8),
            child: Row(
              children: [
                Expanded(
                  child: TextField(
                    controller: _controller,
                    decoration: const InputDecoration(
                      hintText: "Type a message...",
                      border: OutlineInputBorder(),
                    ),
                    onSubmitted: (_) => _send(),
                  ),
                ),
                const SizedBox(width: 8),
                // ✅ Button disabled until connected
                ElevatedButton(
                  onPressed: _connected ? _send : null,
                  child: const Text("Send"),
                ),
              ],
            ),
          ),
        ],
      ),
    );
  }

  @override
  void dispose() {
    _mqtt.disconnect();
    super.dispose();
  }
}