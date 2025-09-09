# Qt Chat Client (C++ / Qt)

Client application for a real-time chat system built with C++ and Qt.  
Connects to the companion server, shows online users and statuses, supports one-on-one chats in tabs, typing indicators, and styled message bubbles with timestamps.

## Features
- TCP connection to server (Qt `QTcpSocket`)
- Login/init handshake via JSON (newline-delimited)
- Live presence: Available, Away, Busy, Offline
- One-to-one chats in dedicated tabs
- Typing indicators
- Message bubbles (mine vs theirs) with timestamps

## Components
- `ClientManager` — networking: connect, init, login, send/receive JSON, typing/status updates
- `MainWindow` — UI: contacts combo, status selector, tabs per conversation, message input
- `ChatItemWidget` — message bubble widget (alignment, color, time)

## Quick Start
1. Clone this repo.
2. Open the `.pro` file in **Qt Creator** (Qt 6.x, Widgets).
3. Build and Run.
4. Ensure the **server** is running (default `localhost:4500`).
5. In the client, choose **Connect** and start chatting.

> The default server address/port are set in `MainWindow::on_actionConnect_triggered()` (LocalHost, 4500). Adjust if needed.

## Protocol (summary)
Messages are newline-delimited JSON. Key packets implemented by the client:

- `{"type":"init","os":"macOS","version":"1.0.0"}`
- `{"type":"login","username":"test","password":"test"}`
- `{"type":"set_name","name":"Alice"}`
- `{"type":"set_status","status":1}`  *(1=Available, 2=Away, 3=Busy, 4=Offline)*
- `{"type":"is_typing","recipient":<id>}`
- `{"type":"message","recipient":<id>,"content":"Hello"}`

Server responses handled:
- `assign_id`, `init_ack`, `login_ack`, `client_list`, `message`, `is_typing`, `set_name`, `set_status`.




