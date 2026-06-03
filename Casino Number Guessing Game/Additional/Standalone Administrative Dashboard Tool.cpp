#include <iostream>
#include <string>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
#else
    #include <unistd.h>
    #include <arpa/inet.h>
    #include <sys/socket.h>
#endif

using namespace std;

int main() {
    cout << "============================================================\n";
    cout << "          🛡️ ENTERPRISE CASINO REMOTE MONITORING SHELL      \n";
    cout << "============================================================\n";

#ifdef _WIN32
    WSADATA wsa; WSAStartup(MAKEWORD(2, 2), &wsa);
#endif

    int adminSocket = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in targetServer;
    targetServer.sin_family = AF_INET;
    targetServer.sin_port = htons(8081); // Remote Admin Authentication Port

#ifdef _WIN32
    InetPton(AF_INET, "127.0.0.1", &targetServer.sin_addr);
#else
    inet_pton(AF_INET, "127.0.0.1", &targetServer.sin_addr);
#endif

    if (connect(adminSocket, (struct sockaddr*)&targetServer, sizeof(targetServer)) < 0) {
        cerr << "❌ [ACCESS ERROR] Root connection rejected. Backend server is either offline or blocked.\n";
        return 1;
    }

    cout << "✅ Security tunnel established. Type /help to view administrative tasks.\n\n";

    char readBuffer[2048];
    while (true) {
        string actionCommand;
        cout << "ADMIN_ROOT@ENGINE_CLUSTER:~# ";
        getline(cin, actionCommand);

        if (actionCommand.empty()) continue;
        if (actionCommand == "/exit") break;

        // Transmit command across administrative socket interface channels
        send(adminSocket, actionCommand.c_str(), actionCommand.length(), 0);

        // Receive telemetry feedback array from the server engine
        memset(readBuffer, 0, sizeof(readBuffer));
#ifdef _WIN32
        int dataReceived = recv(adminSocket, readBuffer, sizeof(readBuffer) - 1, 0);
#else
        int dataReceived = read(adminSocket, readBuffer, sizeof(readBuffer) - 1);
#endif
        if (dataReceived <= 0) {
            cout << "🚨 Server closed the connection channel. Terminal dropping out.\n";
            break;
        }

        cout << readBuffer << "\n";
    }

#ifdef _WIN32
    closesocket(adminSocket); WSACleanup();
#else
    close(adminSocket);
#endif
    return 0;
}
