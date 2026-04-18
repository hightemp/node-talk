# NodeTalk — Product Requirements Document

## 1. Product Overview

**NodeTalk** is a polished, production-ready, cross-platform desktop LAN messenger
for trusted local networks. It is fully peer-to-peer: there is no central server,
no coordinator, and no internet relay. Every machine on the same broadcast
domain (subnet) discovers every other NodeTalk instance and can chat and
exchange files directly with it.

NodeTalk targets small offices, labs, home networks, air-gapped sites,
classrooms and similar trusted-LAN environments where users want a fast,
private, dependency-free way to talk and move files between machines without
relying on any cloud service.

The application is implemented in C++ on Qt 6 (Widgets, Network, Sql),
built with CMake, packaged for Linux, Windows and macOS, and released
through GitHub Releases via GitHub Actions.

## 2. Goals and Non-Goals

### 2.1 Goals
- **Zero-config peer discovery** within a single LAN broadcast domain.
- **1-to-1 text chat** with delivery and read state.
- **1-to-1 file transfer** with chunking, resume, integrity validation
  and a Telegram-desktop-class UX.
- **Persistent local history** in SQLite that survives restarts.
- **Persistent peer identity** independent of IP address.
- **Explicit trust** for new peers (Trust / Reject), persisted locally.
- **Polished single-window desktop UI** built with Qt Widgets that follows
  the host system theme.
- **System tray** integration and minimize-to-tray with continued background
  operation and desktop notifications.
- **Internationalization** with English and Russian shipped from day one and
  **runtime language switching without restart**.
- **Cross-platform**: Linux, Windows, macOS.
- **Repeatable, automated releases** from git tags through GitHub Actions
  producing OS-native artifacts.
- **Production-quality code**: modular, asynchronous, non-blocking UI,
  robust error handling, structured logging, clean resource management,
  cross-platform path handling, and a serious automated test suite.

### 2.2 Non-Goals
- No central server, registration service or coordinator.
- No internet messaging, no NAT traversal, no relays.
- No group chats, channels or multi-party rooms.
- No voice or video calls.
- No multi-user accounts on a single machine; one device = one identity.
- No queueing of messages for offline peers (sending is disallowed).
- No mobile clients.
- No code signing / notarization in the first release cycle.
- No custom in-app theming engine — the application follows the OS theme.

## 3. Architecture Overview

NodeTalk is a single desktop application composed of well-isolated modules:

```
+------------------------------------------------------------------+
|                          UI (Qt Widgets)                         |
|  MainWindow / PeerList / ChatView / Input / Transfers / Tray /   |
|  Settings / TrustDialog / EventLog                               |
+------------------------------------------------------------------+
                  |                              |
+---------------------------------+   +----------------------------+
|         Application core        |   |       i18n / Logging       |
|  Paths, Settings, Identity,     |   |  QTranslator, Logger       |
|  EventLog facade                |   |                            |
+---------------------------------+   +----------------------------+
        |               |
+----------------+  +-----------------+
|   Storage (SQLite)  |   Networking (Qt Network)              |
|  Database +         |  DiscoveryService (UDP bcast/mcast)    |
|  Repositories       |  TransportServer (TCP listen)          |
|  - Peers            |  PeerLink   (per-connection wrapper)   |
|  - Messages         |  PeerManager (lifecycle, routing)      |
|  - Transfers        |  Protocol    (framed JSON + binary)    |
|  - Events           |  FileTransferManager (chunk + resume)  |
|  - Settings         |                                        |
+----------------+    +----------------------------------------+
```

Key design rules:

- Every long-running operation is **asynchronous**. The UI thread never
  blocks on disk or socket I/O.
- **No god objects.** The `Application` only wires modules together; each
  module owns its own state and exposes a narrow Qt-signals interface.
- **One SQLite database per user**, with relational separation by peer
  via foreign keys (no per-dialog database files).
- **Stable identity is decoupled from IP**: a generated UUID + random
  fingerprint pair is the durable peer identity.
- **All sockets** are managed via Qt Network (`QUdpSocket`, `QTcpServer`,
  `QTcpSocket`).

## 4. User Stories

1. *As a user*, when I launch NodeTalk on a fresh machine I see other
   NodeTalk users on my LAN within a few seconds and I can start chatting
   with the trusted ones.
2. *As a user*, when an unknown peer tries to talk to me I see a clear
   "Trust this peer?" prompt before any chat or file transfer happens.
3. *As a user*, I can read all my previous chats with a peer even after
   restarting my computer.
4. *As a user*, I can search through my message history.
5. *As a user*, I can send any file to a trusted online peer with a Telegram
   desktop-style flow: pick file(s), see them in chat with progress, pause,
   resume, cancel.
6. *As a user*, when my Wi-Fi blips during a 2 GB transfer the transfer
   resumes from where it stopped instead of restarting.
7. *As a user*, I can minimize NodeTalk to the system tray and still
   receive desktop notifications for new messages and transfer events.
8. *As a user*, I can switch the UI language between English and Russian
   in Settings and the change is applied immediately, without restarting.
9. *As a user*, I can rename a peer's display name after I trust them.
10. *As a user*, I can manually add a peer by IP address if discovery
    fails because my router blocks broadcasts.
11. *As an operator*, I can install NodeTalk on Linux, Windows and macOS
    from the GitHub Releases page using the artifact appropriate for my OS.

## 5. Functional Requirements

### 5.1 Peer Discovery
- FR-D1: Discovery uses **UDP broadcast** on a configurable port
  (default `45821`) and **UDP multicast** on `239.255.42.99:45822`
  (administratively-scoped multicast address).
- FR-D2: Each instance broadcasts a periodic `announce` every 5 seconds
  and on startup, plus a final `bye` on shutdown.
- FR-D3: A peer is considered **online** while announces are received and
  for `peer_timeout` seconds (default 20) after the last announce.
- FR-D4: A peer is considered **offline** after the timeout elapses but
  remains in the persistent peers list with its last-known nickname,
  IP and trust state, and is rendered in a visually distinct (greyed-out)
  state.
- FR-D5: Discovery messages are JSON over UDP and include `peer_id`,
  `nick`, `tcp_port`, `proto_version`, `fingerprint`.
- FR-D6: Manual peer addition by IP and TCP port must be available from
  the UI; manually-added peers are probed by opening a transient TCP
  connection that performs a `hello` handshake.

### 5.2 Identity and Trust
- FR-I1: On first run, each instance generates a persistent
  `peer_id` (UUID v4) and a random 256-bit `fingerprint`, both stored
  in the local SQLite database.
- FR-I2: The pair `(peer_id, fingerprint)` is the durable identity.
  IP address is transient and not part of identity.
- FR-I3: New peers must be **explicitly trusted** by the local user
  through a "Trust this peer?" dialog before any chat or file transfer
  is allowed.
- FR-I4: Trust state (`unknown`, `trusted`, `blocked`) is persisted and
  survives restarts and IP changes.
- FR-I5: If a peer presents a different `fingerprint` for a previously
  trusted `peer_id`, NodeTalk treats it as untrusted and prompts again.
- FR-I6: Display names of trusted peers can be edited locally; remote
  nickname changes are reflected as a hint but the local label wins.

### 5.3 Chat
- FR-C1: 1-to-1 text messaging only.
- FR-C2: Message history is persisted in SQLite and loaded on demand
  (paged) when a peer's chat is opened.
- FR-C3: Each message has states `sending`, `sent`, `delivered`, `read`,
  `failed`.
- FR-C4: Typing indicator: send `typing(active=true|false)` with
  debouncing while the user is composing.
- FR-C5: Sending to an offline peer is **blocked** in the UI and not
  queued.
- FR-C6: Full-text search across history (case-insensitive substring
  via SQL `LIKE`, optionally upgrade-able to FTS5 if available).

### 5.4 File Transfer
- FR-F1: Direct peer-to-peer transfer over the existing TCP `PeerLink`.
- FR-F2: Any file type, any size limited only by free disk space.
- FR-F3: Telegram-like UX: drop file(s) into the chat input or pick via
  file dialog; each file appears as a chat item with an inline progress
  bar and Pause / Resume / Cancel controls.
- FR-F4: A transfer queue allows scheduling many transfers; up to
  `max_parallel_transfers` (default 3) run simultaneously per peer.
- FR-F5: Chunked transfer with default `chunk_size = 64 KiB`. Each
  chunk is acknowledged by the receiver to support flow control and
  resume.
- FR-F6: Integrity validation via SHA-256 of the full file in the
  `file_offer` and final `file_complete` check.
- FR-F7: The transfer protocol must support resume after disconnects
  by replaying from the last acknowledged chunk; partial files are
  preserved on disk and listed in the transfers UI.
- FR-F8: Receiver can `Accept` (with default or chosen save path) or
  `Reject` an incoming offer.
- FR-F9: Default save folder is per-OS app data, configurable in
  Settings.

### 5.5 Settings
- FR-S1: Display name (local nickname).
- FR-S2: UI language (English / Russian, runtime switchable).
- FR-S3: Default download/save folder (with "open folder" button).
- FR-S4: Tray behavior (start in tray, minimize to tray, close to tray).
- FR-S5: Network options: discovery port, multicast group, TCP port
  (`0` = auto), peer timeout, max parallel transfers.
- FR-S6: Trusted peers management: list, rename, untrust, block.

### 5.6 Tray, Notifications, Event Log
- FR-N1: System tray icon with context menu (Show/Hide, Quit).
- FR-N2: Minimize-to-tray and close-to-tray (configurable).
- FR-N3: The application keeps running in the background while in tray:
  discovery and transfers continue.
- FR-N4: Desktop notifications for new messages, file offers, completed
  / failed transfers (when window is not focused).
- FR-N5: An in-app **Event Log** records: peer discovered, peer lost,
  trust accepted, file offered, file accepted, file paused, file resumed,
  file failed, file completed.

### 5.7 Internationalization
- FR-L1: All user-facing strings are wrapped with `tr()`.
- FR-L2: Translation files `nodetalk_en.ts`, `nodetalk_ru.ts` are stored
  under `resources/i18n/` and compiled to `.qm` at build time.
- FR-L3: Switching language in Settings re-installs the active
  `QTranslator` and emits `LanguageChange` events; every visible widget
  retranslates itself in `changeEvent`.

## 6. Non-Functional Requirements

- NFR-1: Cold start to "ready and discovering" under 1 s on commodity
  hardware.
- NFR-2: UI remains responsive (16 ms frame budget) during 1 GB+ file
  transfers.
- NFR-3: Memory footprint under 200 MB during normal use.
- NFR-4: Crash-free during normal LAN-disconnect / reconnect cycles.
- NFR-5: All disk and network I/O is asynchronous from the UI thread.
- NFR-6: Cross-platform path handling via `QStandardPaths` and `QDir`;
  no hard-coded path separators.
- NFR-7: Structured logging with severity levels via a single
  `Logger` facade backed by `QLoggingCategory`, with a rolling log file
  in the per-OS app data directory.
- NFR-8: Deterministic shutdown: all sockets closed, all pending DB
  writes flushed.
- NFR-9: No third-party C++ dependencies beyond Qt 6 and the system
  SQLite driver bundled with Qt.

## 7. Network Protocol Overview

### 7.1 Discovery (UDP)
JSON datagrams sent to broadcast `255.255.255.255:45821` and to
multicast `239.255.42.99:45822`:

```json
{
  "type": "announce",
  "proto": 1,
  "peer_id": "2c2c9f88-...-uuid",
  "fingerprint": "ab12...32-byte-hex",
  "nick": "alice-laptop",
  "tcp_port": 45823,
  "ts": 1736900000
}
```

A `bye` packet uses the same envelope with `"type":"bye"`.

### 7.2 Transport (TCP)
Each instance runs a `QTcpServer`. A frame is:

```
+--------+--------+----------------+
| len:u32| typ:u8 |   payload      |
+--------+--------+----------------+
```

- `len`  big-endian uint32, length of `payload` in bytes.
- `typ`  `0x01` JSON UTF-8, `0x02` BIN (binary chunk).
- `payload` opaque body.

Maximum frame size is 1 MiB; chunk frames carry the binary chunk body
directly to avoid base64 overhead.

### 7.3 Application Messages

Handshake & control:
- `hello { peer_id, fingerprint, nick, proto, app_version }`
- `hello_ack { peer_id, fingerprint, nick, proto, app_version }`
- `bye {}`

Chat:
- `text { msg_id, ts, body }`
- `text_ack { msg_id, status: "delivered" | "read" }`
- `typing { active: bool }`

File transfer:
- `file_offer { xfer_id, name, size, mime, sha256, chunk_size, chunks }`
- `file_accept { xfer_id, resume_offset }`
- `file_reject { xfer_id, reason }`
- `file_chunk_req { xfer_id, index }`
- `file_chunk_meta { xfer_id, index, size }` immediately followed by
  one BIN frame of `size` bytes.
- `file_pause { xfer_id }`
- `file_resume { xfer_id }`
- `file_complete { xfer_id, ok: bool, sha256_ok: bool }`

The receiver drives flow control by issuing `file_chunk_req` for the
next chunk it wants. This makes pause/resume trivial: the receiver
simply stops requesting chunks. Sender state is therefore minimal and
the protocol naturally survives disconnects: on reconnect the receiver
reissues `file_accept` with a `resume_offset` equal to bytes already
on disk, and the sender re-sends from the corresponding chunk.

## 8. Storage Design (SQLite)

```
peers(
  id           TEXT PRIMARY KEY,        -- peer_id (UUID)
  fingerprint  TEXT NOT NULL,
  nickname     TEXT NOT NULL,
  display_name TEXT,                    -- local override
  trust        INTEGER NOT NULL DEFAULT 0, -- 0 unknown, 1 trusted, 2 blocked
  last_ip      TEXT,
  last_port    INTEGER,
  last_seen    INTEGER,                 -- unix seconds
  created_at   INTEGER NOT NULL
);

messages(
  id        TEXT PRIMARY KEY,           -- msg_id (UUID)
  peer_id   TEXT NOT NULL REFERENCES peers(id) ON DELETE CASCADE,
  direction INTEGER NOT NULL,           -- 0 outgoing, 1 incoming
  body      TEXT NOT NULL,
  ts        INTEGER NOT NULL,
  status    INTEGER NOT NULL,           -- 0 sending,1 sent,2 delivered,3 read,4 failed
  kind      INTEGER NOT NULL DEFAULT 0  -- 0 text, 1 file
);
CREATE INDEX idx_messages_peer_ts ON messages(peer_id, ts);
CREATE INDEX idx_messages_body    ON messages(body);

transfers(
  id          TEXT PRIMARY KEY,         -- xfer_id
  msg_id      TEXT REFERENCES messages(id) ON DELETE SET NULL,
  peer_id     TEXT NOT NULL REFERENCES peers(id),
  direction   INTEGER NOT NULL,
  file_name   TEXT NOT NULL,
  file_size   INTEGER NOT NULL,
  mime        TEXT,
  sha256      TEXT,
  local_path  TEXT NOT NULL,
  bytes_done  INTEGER NOT NULL DEFAULT 0,
  state       INTEGER NOT NULL,         -- 0 pending,1 active,2 paused,3 done,4 failed,5 cancelled
  started_at  INTEGER,
  finished_at INTEGER
);
CREATE INDEX idx_transfers_peer ON transfers(peer_id);

settings(
  key   TEXT PRIMARY KEY,
  value TEXT NOT NULL
);

events(
  id      INTEGER PRIMARY KEY AUTOINCREMENT,
  ts      INTEGER NOT NULL,
  level   INTEGER NOT NULL,             -- 0 info, 1 warn, 2 error
  kind    TEXT NOT NULL,                -- e.g. "peer_discovered"
  peer_id TEXT,
  text    TEXT
);
CREATE INDEX idx_events_ts ON events(ts);
```

The database file lives at:
- Linux  : `~/.local/share/NodeTalk/nodetalk.sqlite`
- Windows: `%APPDATA%\NodeTalk\nodetalk.sqlite`
- macOS  : `~/Library/Application Support/NodeTalk/nodetalk.sqlite`

Schema migrations are tracked by `PRAGMA user_version`.

## 9. Trust Model

- NodeTalk targets **trusted LANs**. Confidentiality on the wire is
  **not** a goal of v1 (the threat model assumes a benign LAN).
- Identity binding is achieved by storing `(peer_id, fingerprint)` once
  the user explicitly trusts a peer. Subsequent connections that present
  the same `peer_id` but a different `fingerprint` are flagged and
  re-prompted, mitigating accidental impersonation when peers move IPs.
- Blocked peers are silently rejected at the `hello` stage.
- Manual peer addition does not bypass trust: a manually-added peer is
  inserted with `trust = unknown` and still requires acceptance.
- A future version may upgrade the link to TLS using a per-device
  self-signed certificate whose fingerprint replaces the random
  `fingerprint` field; the protocol envelope is forward-compatible.

## 10. Packaging and Release Requirements

Required artifacts per release:

- **Windows**: `NodeTalk-<version>-windows-x64.zip` (portable),
  `NodeTalk-<version>-windows-x64-setup.exe` (Inno Setup).
- **Linux**:   `NodeTalk-<version>-x86_64.AppImage`,
  `NodeTalk-<version>-linux-x86_64.tar.gz`,
  `nodetalk_<version>_amd64.deb`.
- **macOS**:   `NodeTalk-<version>-macos.zip` (zipped `.app`),
  `NodeTalk-<version>-macos.dmg`.

Versioning is derived from git tags (`vX.Y.Z`), surfaced to the binary
through a generated `Version.h` produced by CMake at configure time.

GitHub Actions workflows:
- `ci.yml`: builds and runs tests on push / PR for Linux, Windows, macOS.
- `release.yml`: triggered on tags `v*`, builds release artifacts on all
  three OSes, generates release notes from the commit log between the
  previous and current tag, and uploads artifacts to the GitHub Release.

## 11. Testing Strategy

- **Unit tests** (Qt Test) for:
  - `Identity` generation, persistence, fingerprint stability.
  - `Database` schema creation and migration.
  - Repositories CRUD.
  - `Protocol` framing/parsing round-trip and malformed-input handling.
  - `Settings` round-trip.
- **Integration tests**:
  - Two `PeerManager` instances on `localhost` exchanging text and
    completing a file transfer with intentional mid-transfer disconnect
    and resume.
  - `DiscoveryService` round-trip on the loopback (where the OS allows).
- **Smoke tests**:
  - Application starts, opens main window, opens settings, switches
    language at runtime, exits cleanly, in headless `offscreen` mode.
- **CI**:
  - Tests run on every push/PR on all three OSes.
  - `ctest --output-on-failure` is the gate.

## 12. Risks and Tradeoffs

| Risk                                     | Mitigation                                   |
|------------------------------------------|----------------------------------------------|
| Routers blocking UDP broadcast/multicast | Fallback manual peer add by IP.              |
| Public-key crypto not in Qt by default   | v1 uses random fingerprint trust on TPI LAN; TLS upgrade path is reserved in protocol. |
| OS firewalls block TCP port              | App documents required ports; installer optionally adds firewall rule on Windows. |
| Large files saturating event loop        | Chunked, receiver-driven flow control; chunk read/write done in dedicated worker `QObject`s on a `QThread`. |
| SQLite write contention                  | Single writer thread; UI uses async signals to repository slots. |
| Translation drift                        | `lupdate` invoked at build via CMake; missing strings fail soft. |
| Cross-platform tray quirks               | Feature-detect `QSystemTrayIcon::isSystemTrayAvailable()`; gracefully degrade. |
