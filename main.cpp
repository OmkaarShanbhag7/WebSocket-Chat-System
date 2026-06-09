#include "crow_all.h"
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <string>
#include <iostream>
#include <vector>

using namespace std;

struct ChatServer {
	unordered_map<string , unordered_set<crow :: websocket :: connection*>> rooms;

	unordered_map<crow :: websocket :: connection* , string> user_to_rooms;

	//mutex server_mutex;
	const size_t MAX_ROOM_CAPACITY = 7;
};

ChatServer chat_server;
mutex state_mutex;


int main() {
    crow::SimpleApp app;

    CROW_WEBSOCKET_ROUTE(app, "/ws")
        .onopen([&](crow::websocket::connection& conn) {
            CROW_LOG_INFO << "New Client Connected! Address: " << &conn ;
	   // lock_guard<mutex> lock(chat_server.server_mutex);
	    lock_guard<mutex> lock(state_mutex);
	    string default_room = "looby";

	    chat_server.user_to_rooms[&conn] = default_room;

	    chat_server.rooms[default_room].insert(&conn);

	   // CROW_LOG_INFO<< "Client " << &conn << " Successfully locked and loaded into: "<<default_room;
	   //cout<< "[INFO] New connnection established. Assigned to room : [" <<default_room << "]" <<endl;	
 	   conn.send_text(R"({"status": "connected" , "message": "welcome. You are in 'lobby' . Use join_room to switch. "})");
	
	})

        .onclose([&](crow::websocket::connection& conn, const std::string& reason) {
            CROW_LOG_INFO << "Client Disconnected. Reason: " << reason;
	    //lock_guard<mutex> lock(chat_server.server_mutex);
	    lock_guard<mutex> lock(state_mutex);
	    auto it = chat_server.user_to_rooms.find(&conn);
	    if(it != chat_server.user_to_rooms.end()){
		string room_name = it->second;

		chat_server.rooms[room_name].erase(&conn);
		chat_server.user_to_rooms.erase(it);

		if(chat_server.rooms[room_name].empty() && room_name != "lobby"){
			chat_server.rooms.erase(room_name);
		}
	     	cout<<"[INFO] Cleaned uo disconnected user socket handles." <<endl;
	    }
	})
        .onmessage([&](crow::websocket::connection& conn, const std::string& data, bool is_binary) {
        	if(is_binary){
			cout<<"[WARN] Binary frame received. Discarding the message"<<endl;
			return ;
		}
		crow :: json :: rvalue parsed_json;
		try{
			parsed_json = crow::json::load(data);
			if(!parsed_json){
				conn.send_text(R"({"error": "Invalid payload format. Expected JSON."})");
				return;
			}
		}catch(...){
			conn.send_text(R"({"error": "JSON compilation crashed."})");
			return;
		}
		if(!parsed_json.has("action")){
			conn.send_text(R"({"error": "Missing mandatory 'action' protocol parameter."})");
			return;
		}
		
		string action = parsed_json["action"].s();
		
		//ACTION FEATURES:
		if(action == "join_room"){
			if(!parsed_json.has("room_name")){
				conn.send_text(R"({"error: "Missing 'room_name' parameter."})");
				return;
			}
			string target_room = parsed_json["room_name"].s();

			lock_guard<mutex> lock(state_mutex);
			
			if(chat_server.rooms[target_room].size() >= chat_server.MAX_ROOM_CAPACITY){
				conn.send_text(R"({"status" : "denied" , "error": "Target room capacity maxed"})");
				return;
			}

			string old_room = chat_server.user_to_rooms[&conn];
			chat_server.rooms[old_room].erase(&conn);

			chat_server.rooms[target_room].insert(&conn);
			chat_server.user_to_rooms[&conn] = target_room;

			conn.send_text(R"({"status": "success" , "message": "Successfully migrated to room: )" + target_room + R"("})");
		}

		else if(action == "send_message"){
			if(!parsed_json.has("room_name") || !parsed_json.has("message")){
				conn.send_text(R"({"error": "Incomplete transmission payloads."})");
				return;
			}

			string target_room = parsed_json["room_name"].s();
			string msg_content = parsed_json["message"].s();

			vector<crow::websocket::connection*> target_clients;

			{
				lock_guard<mutex> lock(state_mutex);
				auto room_it = chat_server.rooms.find(target_room);
				if(room_it != chat_server.rooms.end()){
					for(auto* client : room_it->second){
						if(client != &conn){
							target_clients.push_back(client);
						}
					}
				}

			}
		
			string bcast = R"({"from_room": ")" + target_room + R"(" , "msg": ")" + msg_content + R"("})";
			for(auto* client : target_clients){
				client->send_text(bcast);
			}
			conn.send_text(R"({"status": "sent"})");
			return;

		}
		else{
			conn.send_text(R"("{"error": "Unknown protocol action identifier."})");
		}
	});
	
	app.port(8080).multithreaded().run();
}



