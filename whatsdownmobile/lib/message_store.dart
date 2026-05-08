import 'package:shared_preferences/shared_preferences.dart';
import 'dart:convert';

class MessageStore {
  final String myId;
  static const int maxMessages = 200;

  MessageStore({required this.myId});

  String get _key => "messages_$myId";

  Future<List<Map<String, String>>> load() async {
    final prefs = await SharedPreferences.getInstance();
    final raw = prefs.getStringList(_key) ?? [];
    return raw.map((e) => Map<String, String>.from(jsonDecode(e))).toList();
  }

  Future<void> save(String sender, String content) async {
    final prefs = await SharedPreferences.getInstance();
    final raw = prefs.getStringList(_key) ?? [];

    raw.add(jsonEncode({
      "sender": sender,
      "content": content,
      "time": DateTime.now().toIso8601String(),
    }));

    final trimmed = raw.length > maxMessages
        ? raw.sublist(raw.length - maxMessages)
        : raw;

    await prefs.setStringList(_key, trimmed);
  }

  Future<void> clear() async {
    final prefs = await SharedPreferences.getInstance();
    await prefs.remove(_key);
  }
}
