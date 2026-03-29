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
  late final MessageStore _store;
  late final MqttService _mqtt;
  final List<Map<String, String>> _messages = [];
  final TextEditingController _controller = TextEditingController();

  @override
  void initState() {
    super.initState();
    _store = MessageStore(myId: widget.myId);
    _mqtt = widget.mqtt;

    // ✅ Load saved history first
    _store.load().then((history) {
      setState(() => _messages.addAll(history));
    });

    // ✅ Save and display every incoming message
    _mqtt.onMessageFromKid = (msg) {
      _store.save("kid", msg);
      setState(() => _messages.add({"sender": "kid", "content": msg}));
    };
  }

  void _send() {
    final text = _controller.text.trim();
    if (text.isEmpty) return;
    _mqtt.sendToKid(text);
    _store.save(widget.myId, text);                           // ✅ save outgoing too
    setState(() => _messages.add({"sender": widget.myId, "content": text}));
    _controller.clear();
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: Text("Chat as ${widget.myId}"),
        actions: [
          // ✅ Optional clear history button
          IconButton(
            icon: const Icon(Icons.delete_outline),
            onPressed: () async {
              await _store.clear();
              setState(() => _messages.clear());
            },
          )
        ],
      ),
      body: Column(
        children: [
          Expanded(
            child: ListView.builder(
              padding: const EdgeInsets.all(12),
              itemCount: _messages.length,
              itemBuilder: (_, i) {
                final msg = _messages[i];
                final isMe = msg["sender"] == widget.myId;
                return Align(
                  alignment: isMe
                      ? Alignment.centerRight
                      : Alignment.centerLeft,
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
                        color: isMe ? Colors.white : Colors.black87,
                      ),
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
      ),
    );
  }
}