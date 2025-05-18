// //
// // Created by mwintrow on 5/17/25.
// //
//
// #include <opencv2/opencv.hpp>
// #include <iostream>
// #include <string>
// #include <thread>
// #include <mutex>
// #include <atomic>
// #include <cstring>
// #include <sys/socket.h>
// #include <netinet/in.h>
// #include <arpa/inet.h>
// #include <unistd.h>
//
// using namespace cv;
// using namespace std;
//
// atomic<bool> isRunning(true);
// atomic<bool> isConnected(false);
// mutex frameMutex;
// Mat remoteFrame;
// string connectionString;
// int serverSocket = -1;
// int clientSocket = -1;
// thread networkThread;
//
// vector<uchar> compressFrame(const Mat& frame) {
//     vector<uchar> buffer;
//     vector<int> params = {IMWRITE_JPEG_QUALITY, 60};
//     imencode(".jpg", frame, buffer, params);
//     return buffer;
// }
//
// Mat decompressFrame(const vector<uchar>& buffer) {
//     return imdecode(buffer, IMREAD_COLOR);
// }
//
// void createServer() {
//     serverSocket = socket(AF_INET, SOCK_STREAM, 0);
//     if (serverSocket < 0) {
//         cerr << "Error creating server socket" << endl;
//         return;
//     }
//
//     int opt = 1;
//     setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
//
//     struct sockaddr_in serverAddr;
//     memset(&serverAddr, 0, sizeof(serverAddr));
//     serverAddr.sin_family = AF_INET;
//     serverAddr.sin_addr.s_addr = INADDR_ANY;
//     serverAddr.sin_port = htons(8888);
//
//     if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
//         cerr << "Error binding server socket" << endl;
//         close(serverSocket);
//         return;
//     }
//
//     cout << "Server started. Connection string: " << endl;
//
//     char hostname[128];
//     gethostname(hostname, sizeof(hostname));
//     struct hostent* host_entry = gethostbyname(hostname);
//     char* localIP = inet_ntoa(*((struct in_addr*)host_entry->h_addr_list[0]));
//
//     connectionString = string(localIP) + ":8888";
//     cout << connectionString << endl;
//
//     struct sockaddr_in clientAddr;
//     socklen_t clientAddrLen = sizeof(clientAddr);
//     clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientAddrLen);
//
//     if (clientSocket < 0) {
//         cerr << "Error accepting connection" << endl;
//         close(serverSocket);
//         return;
//     }
//
//     isConnected = true;
//     cout << "Client connected: " << inet_ntoa(clientAddr.sin_addr) << endl;
// }
//
// void connectToServer(const string& connectionStr) {
//     size_t colonPos = connectionStr.find(":");
//     if (colonPos == string::npos) {
//         cerr << "Invalid connection string" << endl;
//         return;
//     }
//
//     string ip = connectionStr.substr(0, colonPos);
//     int port = stoi(connectionStr.substr(colonPos + 1));
//
//     clientSocket = socket(AF_INET, SOCK_STREAM, 0);
//     if (clientSocket < 0) {
//         cerr << "Error creating client socket" << endl;
//         return;
//     }
//
//     struct sockaddr_in serverAddr;
//     memset(&serverAddr, 0, sizeof(serverAddr));
//     serverAddr.sin_family = AF_INET;
//     serverAddr.sin_port = htons(port);
//
//     if (inet_pton(AF_INET, ip.c_str(), &serverAddr.sin_addr) <= 0) {
//         cerr << "Invalid IP address" << endl;
//         close(clientSocket);
//         return;
//     }
//
//     if (connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
//         cerr << "Error connecting to server" << endl;
//         close(clientSocket);
//         return;
//     }
//
//     isConnected = true;
//     cout << "Client connected: " << ip << ":" << port << endl;
// }
//
// void networkFunction() {
//     while (isRunning) {
//         if (!isConnected) {
//             this_thread::sleep_for(chrono::milliseconds(100));
//             continue;
//         }
//
//         // Receive frame size
//         uint32_t frameSize;
//         int bytesRead = recv(clientSocket, &frameSize, sizeof(frameSize), 0);
//         if (bytesRead <= 0) {
//             cerr << "Connection closed or error" << endl;
//             isConnected = false;
//             close(clientSocket);
//             if (serverSocket != -1) {
//                 close(serverSocket);
//                 serverSocket = -1;
//             }
//             continue;
//         }
//
//         // Receive frame data
//         vector<uchar> buffer(frameSize);
//         int totalBytesRead = 0;
//         while (totalBytesRead < frameSize) {
//             bytesRead = recv(clientSocket, buffer.data() + totalBytesRead,
//                             frameSize - totalBytesRead, 0);
//             if (bytesRead <= 0) {
//                 cerr << "Connection closed or error while receiving frame" << endl;
//                 isConnected = false;
//                 break;
//             }
//             totalBytesRead += bytesRead;
//         }
//
//         if (!isConnected) {
//             continue;
//         }
//
//         // Decompress and store frame
//         Mat receivedFrame = decompressFrame(buffer);
//         if (!receivedFrame.empty()) {
//             lock_guard<mutex> lock(frameMutex);
//             remoteFrame = receivedFrame.clone();
//         }
//     }
// }
//
// int main() {
//     // Initialize camera
//     VideoCapture camera(0);
//     if (!camera.isOpened()) {
//         cerr << "Error: Could not open camera" << endl;
//         return -1;
//     }
//
//     // Create windows
//     namedWindow("Local Camera", WINDOW_NORMAL);
//     namedWindow("Remote Camera", WINDOW_NORMAL);
//
//     // Start network thread
//     networkThread = thread(networkFunction);
//
//     // Main loop
//     while (isRunning) {
//         // Capture local frame
//         Mat localFrame;
//         camera >> localFrame;
//         if (localFrame.empty()) {
//             cerr << "Error: Empty frame from camera" << endl;
//             break;
//         }
//
//         // Display local frame
//         imshow("Local Camera", localFrame);
//
//         // Display remote frame if available
//         {
//             lock_guard<mutex> lock(frameMutex);
//             if (!remoteFrame.empty()) {
//                 imshow("Remote Camera", remoteFrame);
//             }
//         }
//
//         // Send local frame if connected
//         if (isConnected) {
//             vector<uchar> compressedFrame = compressFrame(localFrame);
//             uint32_t frameSize = compressedFrame.size();
//
//             // Send frame size
//             send(clientSocket, &frameSize, sizeof(frameSize), 0);
//
//             // Send frame data
//             send(clientSocket, compressedFrame.data(), compressedFrame.size(), 0);
//         }
//
//         // Handle keyboard input
//         int key = waitKey(30);
//         if (key == 27) { // ESC key
//             isRunning = false;
//             break;
//         } else if (key == 's') { // 's' key to start server
//             if (!isConnected && serverSocket == -1) {
//                 thread serverThread(createServer);
//                 serverThread.detach();
//             }
//         } else if (key == 'c') { // 'c' key to connect to server
//             if (!isConnected) {
//                 string input;
//                 cout << "Enter connection string: ";
//                 cin >> input;
//                 thread clientThread([input]() { connectToServer(input); });
//                 clientThread.detach();
//             }
//         } else if (key == 'd') { // 'd' key to disconnect
//             if (isConnected) {
//                 isConnected = false;
//                 close(clientSocket);
//                 if (serverSocket != -1) {
//                     close(serverSocket);
//                     serverSocket = -1;
//                 }
//                 cout << "Disconnected" << endl;
//             }
//         }
//
//         // Show instructions
//         Mat instructionsFrame = Mat::zeros(100, 500, CV_8UC3);
//         putText(instructionsFrame, "ESC: Exit  |  S: Start Call  |  C: Join Call  |  D: End Call",
//                 Point(10, 50), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(255, 255, 255), 1);
//         imshow("Instructions", instructionsFrame);
//     }
//
//     // Cleanup
//     isRunning = false;
//     if (isConnected) {
//         close(clientSocket);
//         if (serverSocket != -1) {
//             close(serverSocket);
//         }
//     }
//
//     if (networkThread.joinable()) {
//         networkThread.join();
//     }
//
//     camera.release();
//     destroyAllWindows();
//
//     return 0;
// }

#include <opencv2/opencv.hpp>
#include <iostream>
#include <string>
#include <thread>
#include <mutex>
#include <atomic>
#include <vector>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <unistd.h>

using namespace cv;
using namespace std;

// Global variables
atomic<bool> isRunning(true);
atomic<bool> isConnected(false);
mutex frameMutex;
Mat remoteFrame;
string connectionString;
int serverSocket = -1;
int clientSocket = -1;
thread networkThread;

// Get the local IP address (non-loopback)
string getLocalIP() {
    struct ifaddrs *ifaddr, *ifa;
    string ip = "127.0.0.1"; // Default to localhost

    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        return ip;
    }

    // Walk through linked list, finding the first non-loopback IPv4 address
    for (ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == nullptr) continue;

        if (ifa->ifa_addr->sa_family == AF_INET) {
            void* tmpAddrPtr = &((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
            char addressBuffer[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);

            // Skip localhost addresses
            if (strcmp(addressBuffer, "127.0.0.1") != 0) {
                ip = addressBuffer;
                break;
            }
        }
    }

    freeifaddrs(ifaddr);
    return ip;
}

// Function to compress a frame for network transmission
vector<uchar> compressFrame(const Mat& frame) {
    vector<uchar> buffer;
    vector<int> params = {IMWRITE_JPEG_QUALITY, 60};
    imencode(".jpg", frame, buffer, params);
    return buffer;
}

// Function to decompress a frame received from network
Mat decompressFrame(const vector<uchar>& buffer) {
    return imdecode(buffer, IMREAD_COLOR);
}

// Function to create a server and wait for connections
void createServer() {
    // Create socket
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        cerr << "Error creating socket" << endl;
        return;
    }

    // Allow reuse of address
    int opt = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // Bind to port
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(8888);

    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        cerr << "Error binding socket" << endl;
        close(serverSocket);
        return;
    }

    // Listen for connections
    if (listen(serverSocket, 1) < 0) {
        cerr << "Error listening" << endl;
        close(serverSocket);
        return;
    }

    // Get local IP address
    string localIP = getLocalIP();

    // Create connection string
    connectionString = localIP + ":8888";
    cout << "Server started. Connection string: " << connectionString << endl;

    // Accept connection
    struct sockaddr_in clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);
    cout << "Waiting for someone to connect..." << endl;
    clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientAddrLen);

    if (clientSocket < 0) {
        cerr << "Error accepting connection" << endl;
        close(serverSocket);
        return;
    }

    isConnected = true;
    cout << "Client connected: " << inet_ntoa(clientAddr.sin_addr) << endl;
}

// Function to connect to a server
void connectToServer(const string& connectionStr) {
    // Parse connection string
    size_t colonPos = connectionStr.find(':');
    if (colonPos == string::npos) {
        cerr << "Invalid connection string" << endl;
        return;
    }

    string ip = connectionStr.substr(0, colonPos);
    int port = stoi(connectionStr.substr(colonPos + 1));

    // Create socket
    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket < 0) {
        cerr << "Error creating socket" << endl;
        return;
    }

    // Connect to server
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);

    if (inet_pton(AF_INET, ip.c_str(), &serverAddr.sin_addr) <= 0) {
        cerr << "Invalid address" << endl;
        close(clientSocket);
        return;
    }

    cout << "Connecting to " << ip << ":" << port << "..." << endl;
    if (connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        cerr << "Connection failed" << endl;
        close(clientSocket);
        return;
    }

    isConnected = true;
    cout << "Connected to " << ip << ":" << port << endl;
}

// Function to send and receive video frames
void networkFunction() {
    while (isRunning) {
        if (!isConnected) {
            this_thread::sleep_for(chrono::milliseconds(100));
            continue;
        }

        // Receive frame size
        uint32_t frameSize;
        int bytesRead = recv(clientSocket, &frameSize, sizeof(frameSize), 0);
        if (bytesRead <= 0) {
            cerr << "Connection closed or error" << endl;
            isConnected = false;
            close(clientSocket);
            if (serverSocket != -1) {
                close(serverSocket);
                serverSocket = -1;
            }
            continue;
        }

        // Receive frame data
        vector<uchar> buffer(frameSize);
        int totalBytesRead = 0;
        while (totalBytesRead < frameSize) {
            bytesRead = recv(clientSocket, buffer.data() + totalBytesRead,
                            frameSize - totalBytesRead, 0);
            if (bytesRead <= 0) {
                cerr << "Connection closed or error while receiving frame" << endl;
                isConnected = false;
                break;
            }
            totalBytesRead += bytesRead;
        }

        if (!isConnected) {
            continue;
        }

        // Decompress and store frame
        Mat receivedFrame = decompressFrame(buffer);
        if (!receivedFrame.empty()) {
            lock_guard<mutex> lock(frameMutex);
            remoteFrame = receivedFrame.clone();
        }
    }
}

int main() {
    // Initialize camera
    VideoCapture camera(0);
    if (!camera.isOpened()) {
        cerr << "Error: Could not open camera" << endl;
        return -1;
    }

    // Create windows
    namedWindow("Local Camera", WINDOW_NORMAL);
    namedWindow("Remote Camera", WINDOW_NORMAL);
    namedWindow("Instructions", WINDOW_NORMAL);

    // Start network thread
    networkThread = thread(networkFunction);

    // Main loop
    while (isRunning) {
        // Capture local frame
        Mat localFrame;
        camera >> localFrame;
        if (localFrame.empty()) {
            cerr << "Error: Empty frame from camera" << endl;
            break;
        }

        // Display local frame
        imshow("Local Camera", localFrame);

        // Display remote frame if available
        {
            lock_guard<mutex> lock(frameMutex);
            if (!remoteFrame.empty()) {
                imshow("Remote Camera", remoteFrame);
            }
        }

        // Send local frame if connected
        if (isConnected) {
            vector<uchar> compressedFrame = compressFrame(localFrame);
            uint32_t frameSize = compressedFrame.size();

            // Send frame size
            send(clientSocket, &frameSize, sizeof(frameSize), 0);

            // Send frame data
            send(clientSocket, compressedFrame.data(), compressedFrame.size(), 0);
        }

        // Handle keyboard input
        int key = waitKey(30);
        if (key == 27) { // ESC key
            isRunning = false;
            break;
        } else if (key == 's' || key == 'S') { // 's' key to start server
            if (!isConnected && serverSocket == -1) {
                thread serverThread(createServer);
                serverThread.detach();
            }
        } else if (key == 'c' || key == 'C') { // 'c' key to connect to server
            if (!isConnected) {
                string input;
                cout << "Enter connection string: ";
                cin >> input;
                thread clientThread([input]() { connectToServer(input); });
                clientThread.detach();
            }
        } else if (key == 'd' || key == 'D') { // 'd' key to disconnect
            if (isConnected) {
                isConnected = false;
                close(clientSocket);
                if (serverSocket != -1) {
                    close(serverSocket);
                    serverSocket = -1;
                }
                cout << "Disconnected" << endl;
            }
        }

        // Show instructions
        Mat instructionsFrame = Mat::zeros(100, 500, CV_8UC3);
        putText(instructionsFrame, "ESC: Exit  |  S: Start Call  |  C: Join Call  |  D: End Call",
                Point(10, 50), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(255, 255, 255), 1);
        imshow("Instructions", instructionsFrame);
    }

    // Cleanup
    isRunning = false;
    if (isConnected) {
        close(clientSocket);
        if (serverSocket != -1) {
            close(serverSocket);
        }
    }

    if (networkThread.joinable()) {
        networkThread.join();
    }

    camera.release();
    destroyAllWindows();

    return 0;
}
