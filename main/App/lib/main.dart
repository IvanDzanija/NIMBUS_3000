import 'package:flutter/material.dart';

void main() => runApp(WhatsDownApp());

class WhatsDownApp extends StatelessWidget {
  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      debugShowCheckedModeBanner: false,
      title: "What's Down Parent",
      theme: ThemeData(
        primaryColor: Colors.orangeAccent,
        scaffoldBackgroundColor: Color(0xFFF5F5F5),
      ),
      home: ParentChatScreen(),
    );
  }
}

class ParentChatScreen extends StatefulWidget {
  @override
  _ParentChatScreenState createState() => _ParentChatScreenState();
}

class _ParentChatScreenState extends State<ParentChatScreen> {
  // Lista je sada prazna na početku - čeka podatke s ESP32-S3
  final List<Map<String, dynamic>> _messages = [];
  
  final TextEditingController _controller = TextEditingController();

  // 1. FUNKCIJA: Kada RODITELJ šalje djetetu (Publish)
  void _sendMessage(String text) {
    if (text.trim().isEmpty) return;
    setState(() {
      _messages.insert(0, {
        "text": text, 
        "isParent": true, 
        "time": "${DateTime.now().hour}:${DateTime.now().minute.toString().padLeft(2, '0')}"
      });
    });
    _controller.clear();
    // Ovdje ide poziv vašem MQTT klijentu: 
    // mqttClient.publish("whatsdown/child/inbox", text);
  }

  // 2. FUNKCIJA: Kada DIJETE šalje roditelju (Subscribe)
  // Ovu funkciju će pozvati tvoj MQTT klijent unutar "listen" metode
  void _receiveFromChild(String incomingText) {
    setState(() {
      _messages.insert(0, {
        "text": incomingText, 
        "isParent": false, 
        "time": "${DateTime.now().hour}:${DateTime.now().minute.toString().padLeft(2, '0')}"
      });
    });
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        backgroundColor: Colors.orangeAccent,
        elevation: 2,
        title: Row(
          children: [
            CircleAvatar(backgroundColor: Colors.white, child: Text("🦖")),
            SizedBox(width: 12),
            Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Text("Dječji Amulet", style: TextStyle(fontSize: 18, fontWeight: FontWeight.bold, color: Colors.brown[900])),
                Text("Sustav spreman", style: TextStyle(fontSize: 12, color: Colors.green[900])),
              ],
            ),
          ],
        ),
      ),
      body: Column(
        children: [
          // PROZOR ZA PORUKE - Prikazuje se samo ako ima poruka
          Expanded(
            child: _messages.isEmpty 
              ? _buildEmptyState() // Prikazuje se dok dijete ne pošalje prvu poruku
              : ListView.builder(
                  reverse: true,
                  padding: EdgeInsets.all(15),
                  itemCount: _messages.length,
                  itemBuilder: (context, index) {
                    final msg = _messages[index];
                    return _buildChatBubble(msg['text'], msg['isParent'], msg['time']);
                  },
                ),
          ),

          // BRZE NAREDBE (Roditelj šalje piktogram djetetu)
          Container(
            padding: EdgeInsets.symmetric(vertical: 8),
            color: Colors.white,
            child: SingleChildScrollView(
              scrollDirection: Axis.horizontal,
              child: Row(
                children: [
                  _quickAction("Ručak! 🍎"),
                  _quickAction("Dođi doma 🏠"),
                  _quickAction("Laku noć 🌙"),
                  _quickAction("Zovi me 📞"),
                  _quickAction("Bravo! 🌟"),
                ],
              ),
            ),
          ),

          // UNOS TEKSTA
          Container(
            padding: EdgeInsets.all(10),
            decoration: BoxDecoration(color: Colors.white, boxShadow: [BoxShadow(color: Colors.black12, blurRadius: 4)]),
            child: Row(
              children: [
                Expanded(
                  child: TextField(
                    controller: _controller,
                    decoration: InputDecoration(
                      hintText: "Napiši djetetu...",
                      filled: true,
                      fillColor: Colors.grey[100],
                      border: OutlineInputBorder(borderRadius: BorderRadius.circular(30), borderSide: BorderSide.none),
                      contentPadding: EdgeInsets.symmetric(horizontal: 20, vertical: 10),
                    ),
                  ),
                ),
                SizedBox(width: 8),
                FloatingActionButton(
                  onPressed: () => _sendMessage(_controller.text),
                  backgroundColor: Colors.orangeAccent,
                  child: Icon(Icons.send, color: Colors.white),
                  mini: true,
                ),
              ],
            ),
          ),
        ],
      ),
    );
  }

  // Prikaz kada nema poruka (na početku)
  Widget _buildEmptyState() {
    return Center(
      child: Column(
        mainAxisAlignment: MainAxisAlignment.center,
        children: [
          Text("🦕", style: TextStyle(fontSize: 60)),
          SizedBox(height: 10),
          Text("Čekam prvu poruku od djeteta...", style: TextStyle(color: Colors.grey[600], fontStyle: FontStyle.italic)),
        ],
      ),
    );
  }

  Widget _buildChatBubble(String text, bool isParent, String time) {
    return Align(
      alignment: isParent ? Alignment.centerRight : Alignment.centerLeft,
      child: Container(
        margin: EdgeInsets.symmetric(vertical: 6),
        padding: EdgeInsets.symmetric(horizontal: 16, vertical: 10),
        constraints: BoxConstraints(maxWidth: MediaQuery.of(context).size.width * 0.7),
        decoration: BoxDecoration(
          color: isParent ? Colors.orangeAccent[100] : Colors.white,
          borderRadius: BorderRadius.only(
            topLeft: Radius.circular(20),
            topRight: Radius.circular(20),
            bottomLeft: isParent ? Radius.circular(20) : Radius.circular(0),
            bottomRight: isParent ? Radius.circular(0) : Radius.circular(20),
          ),
          boxShadow: [BoxShadow(color: Colors.black.withOpacity(0.05), blurRadius: 3)],
        ),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text(text, style: TextStyle(fontSize: 16, color: Colors.brown[900])),
            SizedBox(height: 4),
            Text(time, style: TextStyle(fontSize: 10, color: Colors.brown[400])),
          ],
        ),
      ),
    );
  }

  Widget _quickAction(String label) {
    return Padding(
      padding: const EdgeInsets.symmetric(horizontal: 6),
      child: ActionChip(
        label: Text(label),
        onPressed: () => _sendMessage(label),
        backgroundColor: Colors.orange[50],
      ),
    );
  }
}