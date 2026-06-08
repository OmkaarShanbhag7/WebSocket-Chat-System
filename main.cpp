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
	   cout<< "[INFO] New connnection established. Assigned to room : [" <<default_room << "]" <<endl;	
 		
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

		if(chat_server.rooms[room_name].empty()){
			chat_server.rooms.erase(room_name);
		}
	     	cout<<"[INFO] Connection closed safely. Cleaned up tracking memory maps." <<endl;
	    }
	})
        .onmessage([&](crow::websocket::connection& conn, const std::string& data, bool is_binary) {
        	if(is_binary){
			cout<<"[WARN] Binary frame received. Discarding the message"<<endl;
			return ;
		}

		vector<crow :: websocket :: connection*> clients_in_room;

		{
			//lock_guard<mutex> lock(chat_server.server_mutex);
			lock_guard<mutex> lock(state_mutex);
			auto it = chat_server.user_to_rooms.find(&conn);
			if(it != chat_server.user_to_rooms.end()){
				string room_name = it->second;

				for(auto* client : chat_server.rooms[room_name]){
					if(client != &conn){
						clients_in_room.push_back(client);
					}
				}
			}
		}
		string broadcast_msg = "User: " + data;
		for(auto* client : clients_in_room){
				client->send_text(broadcast_msg);
			}
		conn.send_text("Server Echo: " + data);

	});
	
	app.port(8080).multithreaded().run();
}



