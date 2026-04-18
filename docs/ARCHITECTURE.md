# Architecture

NodeTalk is a single-process Qt 6 desktop application. It has no server
component. Every running instance is a peer.

```
                           +-------------------+
                           |   ui::MainWindow  |
                           +---------+---------+
                                     |
            +---------+--------------+--------------+----------+
            |         |                             |          |
       PeerListView ChatView      MessageInput  TransferPanel  EventLog
            |                             ^                    ^
            |                             |                    |
            v                             |                    |
       PeerListModel                      |              EventRepository
            |                             |                    ^
            |                             |                    |
            +-> net::PeerManager <--------+--+                 |
                       |                     |                 |
        +--------------+----------+          |          +------+------+
        |              |          |          v          | core::      |
   net::TransportSrv  net::Disc.  PeerLink (TCP)        | EventLog    |
        |              |          ^                     +-------------+
        |              |          |
        +-> QTcpServer +-> QUdpSocket
                                  |
                                  v
                          net::FileTransferManager
                                  |
                                  v
                            db::Database (SQLite)
```

## Module overview

| Layer | Module | Responsibility |
| --- | --- | --- |
| `app` | `Application` | Wires all modules. Owns translator. |
| `app` | `Paths`, `Logger` | Per-OS data dir + rotating log file. |
| `core` | `Identity` | Persistent (UUID, fingerprint). |
| `core` | `Settings` | Typed accessors over the `settings` table; emits change signals. |
| `core` | `EventLog` | Convenience wrapper around `EventRepository`. |
| `db` | `Database` | Connection + migrations (PRAGMA `user_version`). |
| `db` | `*Repository` | CRUD for peers / messages / transfers / events. |
| `model` | `Peer`, `Message`, `Transfer` | Plain data structs + enums. |
| `net` | `Protocol` | Frame codec + `FrameReader`. |
| `net` | `DiscoveryService` | UDP broadcast + multicast announce / bye. |
| `net` | `TransportServer` | `QTcpServer` accepting incoming peers. |
| `net` | `PeerLink` | Wraps a single TCP socket: hello handshake → ready. |
| `net` | `PeerManager` | Brain: peer cache, trust, message routing. |
| `net` | `FileTransferManager` | Receiver-driven chunked file protocol. |
| `ui` | `MainWindow` + widgets | Qt Widgets UI; trust prompt; tray. |

## Threading model

Everything runs on the main thread, driven by Qt's event loop. SQLite
writes are short and synchronous; for very large file transfers chunks
are read incrementally inside `FileTransferManager` so the UI stays
responsive. There is no manual `QThread` usage.

## Persistence

A single SQLite database under
`<QStandardPaths::AppLocalDataLocation>/nodetalk.sqlite` holds:

* `peers` — id, fingerprint, nickname, display_name, trust, last_ip…
* `messages` — id, peer_id, direction, status, kind, body, ts
* `transfers` — id, peer_id, direction, state, file_name/size, sha256…
* `settings` — key/value
* `events` — append-only event log

WAL is enabled. Schema is versioned in `PRAGMA user_version`.

## Identity & trust

On first run an instance generates a random UUID v4 (`peer_id`) plus a
32-byte secret, stored hex-encoded as `fingerprint`. Both are sent in
the `hello` handshake. The receiver stores the pair (peer_id,
fingerprint). On any later reconnect the pair must match exactly,
otherwise a trust prompt is shown.

Three trust states: **Unknown**, **Trusted**, **Blocked**. Only
`Trusted` peers may exchange chat / file traffic; non-handshake frames
from anyone else are dropped and an event is logged.

## Simultaneous-open avoidance

When two peers discover each other simultaneously they would both dial.
We resolve this deterministically: only the side with the
**lexicographically smaller** `peer_id` initiates outbound. The other
side waits. Inbound connections are always accepted; if a duplicate
arrives it replaces the older link.

## Localization

`Application` owns one `QTranslator`. When `Settings::language` changes
it loads the matching `.qm` file and re-installs it; widgets respond to
`QEvent::LanguageChange` by re-running their `retranslateUi()`. No
restart required.
