#include "crow_all.h"

int main() {
    crow::SimpleApp app;

    CROW_ROUTE(app, "/ws")
        .websocket(&app)
        .onopen([&](crow::websocket::connection& conn) {
            CROW_LOG_INFO << "New Client Connected!";
        })
        .onclose([&](crow::websocket::connection& conn, const std::string& reason) {
            CROW_LOG_INFO << "Client Disconnected. Reason: " << reason;
        })
        .onmessage([&](crow::websocket::connection& conn, const std::string& data, bool is_binary) {
            if (!is_binary) {
                conn.send_text("Server Echo: " + data);
            }
        });

    app.port(8080).multithreaded().run();
}
