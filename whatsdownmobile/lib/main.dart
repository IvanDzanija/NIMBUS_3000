import 'package:flutter/material.dart';
import 'mqtt_service.dart';
import 'message_store.dart';

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

enum ConnectionState { connecting, connected, error }

class _ChatScreenState extends State<ChatScreen> {
  late final MessageStore _store;
  late final MqttService _mqtt;
  final List<Map<String, String>> _messages = [];
  final TextEditingController _controller = TextEditingController();

  ConnectionState _connState = ConnectionState.connecting;
  String _errorMessage = "";

  @override
  void initState() {
    super.initState();
    _store = MessageStore(myId: widget.myId);
    _mqtt = MqttService(
      brokerIp: "172.16.55.93",
      myId: widget.myId,
    );

    _mqtt.onConnected = () async {
      // Load history only once we're connected and ready
      final history = await _store.load();
      if (mounted) {
        setState(() {
          _messages.addAll(history);
          _connState = ConnectionState.connected;
        });
      }
    };

    _mqtt.onDisconnected = () {
      if (mounted) {
        setState(() => _connState = ConnectionState.connecting);
        // Auto-retry after 3 seconds
        Future.delayed(const Duration(seconds: 3), _connect);
      }
    };

    _mqtt.onError = (e) {
      if (mounted) {
        setState(() {
          _errorMessage = e;
          _connState = ConnectionState.error;
        });
      }
    };

    _mqtt.onMessageFromKid = (msg) {
      _store.save("kid", msg);
      if (mounted) {
        setState(() => _messages.add({
          "sender": "kid",
          "content": msg,
          "time": DateTime.now().toIso8601String(),
        }));
      }
    };

    _connect();
  }

  void _connect() {
    setState(() {
      _connState = ConnectionState.connecting;
      _errorMessage = "";
    });
    _mqtt.connect();
  }

  void _send() {
    final text = _controller.text.trim();
    if (text.isEmpty || _connState != ConnectionState.connected) return;
    _mqtt.sendToKid(text);
    _store.save(widget.myId, text);
    setState(() => _messages.add({
      "sender": widget.myId,
      "content": text,
      "time": DateTime.now().toIso8601String(),
    }));
    _controller.clear();
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: Text("Chat as ${widget.myId}"),
        actions: [
          IconButton(
            icon: const Icon(Icons.delete_outline),
            onPressed: _connState == ConnectionState.connected
                ? () async {
                    await _store.clear();
                    setState(() => _messages.clear());
                  }
                : null,
          )
        ],
      ),
      body: switch (_connState) {
        ConnectionState.connecting => _buildConnecting(),
        ConnectionState.error     => _buildError(),
        ConnectionState.connected => _buildChat(),
      },
    );
  }

  Widget _buildConnecting() {
    return const Center(
      child: Column(
        mainAxisSize: MainAxisSize.min,
        children: [
          CircularProgressIndicator(),
          SizedBox(height: 16),
          Text("Connecting to broker...", style: TextStyle(color: Colors.grey)),
        ],
      ),
    );
  }

  Widget _buildError() {
    return Center(
      child: Padding(
        padding: const EdgeInsets.all(24),
        child: Column(
          mainAxisSize: MainAxisSize.min,
          children: [
            const Icon(Icons.wifi_off, size: 48, color: Colors.red),
            const SizedBox(height: 16),
            const Text("Could not connect",
                style: TextStyle(fontSize: 18, fontWeight: FontWeight.bold)),
            const SizedBox(height: 8),
            Text(_errorMessage,
                style: const TextStyle(color: Colors.grey), textAlign: TextAlign.center),
            const SizedBox(height: 24),
            ElevatedButton.icon(
              onPressed: _connect,
              icon: const Icon(Icons.refresh),
              label: const Text("Retry"),
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildChat() {
    return Column(
      children: [
        Expanded(
          child: _messages.isEmpty
              ? const Center(
                  child: Text("No messages yet", style: TextStyle(color: Colors.grey)),
                )
              : ListView.builder(
                  padding: const EdgeInsets.all(12),
                  itemCount: _messages.length,
                  itemBuilder: (_, i) {
                    final msg = _messages[i];
                    final isMe = msg["sender"] == widget.myId;
                    return Align(
                      alignment:
                          isMe ? Alignment.centerRight : Alignment.centerLeft,
                      child: Container(
                        margin: const EdgeInsets.symmetric(vertical: 3),
                        padding: const EdgeInsets.symmetric(
                            horizontal: 12, vertical: 8),
                        decoration: BoxDecoration(
                          color: isMe ? Colors.blue[600] : Colors.grey[200],
                          borderRadius: BorderRadius.circular(12),
                        ),
                        child: Text(
                          msg["content"] ?? "",
                          style: TextStyle(
                              color: isMe ? Colors.white : Colors.black87),
                        ),
                      ),
                    );
                  },
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
              ElevatedButton(
                onPressed: _send,
                child: const Text("Send"),
              ),
            ],
          ),
        ),
      ],
    );
  }

  @override
  void dispose() {
    _mqtt.disconnect();
    _controller.dispose();
    super.dispose();
  }
}