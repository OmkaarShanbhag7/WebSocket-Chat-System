#include "crow_all.h"
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <string>

using namespace std;

struct ChatServer {
	unordered_map<string , unordered_set<crow :: websocket :: connection*>> rooms;

	unordered_map<crow :: websocket :: connection* , string> user_to_rooms;

	mutex server_mutex;
};

ChatServer chat_server;

int main() {
    crow::SimpleApp app;

    CROW_ROUTE(app, "/ws")
        .websocket(&app)
        .onopen([&](crow::websocket::connection& conn) {
            CROW_LOG_INFO << "New Client Connected! Address: " << &conn ;
	    lock_guard<mutex> lock(chat_server.server_mutex);
	    string default_room = "looby";

	    chat_server.user_to_rooms[&conn] = default_room;

	    chat_server.rooms[default_room].insert(&conn);

	    CROW_LOG_INFO<< "Client " << &conn << " Successfully locked and loaded into: "<<default_room;

        })
        .onclose([&](crow::websocket::connection& conn, const std::string& reason) {
            CROW_LOG_INFO << "Client Disconnected. Reason: " << reason;
	    lock_guard<mutex> lock(chat_server.server_mutex);
	    if(chat_server.user_to_rooms.count(&conn)){
	    	string room_name = chat_server.user_to_rooms[&conn];
		
		chat_server.rooms[room_name].erase(&conn);
		chat_server.user_to_rooms.erase(&conn);

		CROW_LOG_INFO<< "Clean-up Successful. Removed Pointer " << &conn << "from: " << room_name;
 		}
	})
        .onmessage([&](crow::websocket::connection& conn, const std::string& data, bool is_binary) {
        	if(is_binary) return;

		string user_room = "";
		unordered_set<crow :: websocket :: connection*> clients_in_room;

		{
			lock_guard<mutex> lock(chat_server.server_mutex);
			if(chat_server.user_to_rooms.count(&conn)){
			user_room = chat_server.user_to_rooms[&conn];
			clients_in_room = chat_server.rooms[user_room];
			}
		}

		CROW_LOG_INFO << "User " << &conn << " sent message to room [" << user_room << "]";

		for(auto& client : clients_in_room){
			if(client != &conn){
				client->send_text("Room [" + user_room + "] : " + data);
				}
			}
		});
	app.port(8080).multithreaded().run();
}
