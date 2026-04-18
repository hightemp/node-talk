# NodeTalk — Implementation Plan (TASKS.md)

This file is the single source of truth for the implementation plan.
Tasks are small and actionable. Each task lists its dependencies, its
acceptance criteria and the milestone it belongs to. The order in this
document is also the recommended execution order.

Legend:
- **D:** dependencies (`-` if none)
- **AC:** acceptance criteria
- **M:** milestone (`M1 = MVP`, `M2 = Final release`)

---

## Milestone M1 — MVP
Goal: two NodeTalk instances on the same LAN can discover each other,
trust each other, exchange text messages persisted in SQLite, and
transfer at least one file end-to-end.

### T-001 Repository scaffolding
- D: -
- AC:
  - `node-talk/` contains `README.md`, `LICENSE`, `CHANGELOG.md`,
    `.gitignore`, `PRD.md`, `TASKS.md`.
  - `docs/` contains placeholder docs.
- M: M1

### T-002 CMake build system
- D: T-001
- AC:
  - Top-level `CMakeLists.txt` finds Qt 6 (`Widgets`, `Network`, `Sql`,
    `LinguistTools`, `Test`).
  - Builds an empty `nodetalk` executable target.
  - Generates `Version.h` from git tag (or `v0.0.0+dev` fallback).
  - `cmake --build` succeeds on Linux/Windows/macOS.
- M: M1

### T-003 Resources & translations skeleton
- D: T-002
- AC:
  - `resources/nodetalk.qrc` exists.
  - `resources/i18n/nodetalk_en.ts` and `nodetalk_ru.ts` exist.
  - CMake compiles `.ts` to `.qm` using `qt_add_translations` /
    `qt_add_lupdate` and embeds them.
- M: M1

### T-004 Application bootstrap
- D: T-002
- AC:
  - `src/main.cpp` creates `QApplication`, sets org/app names,
    instantiates `Application`, shows `MainWindow`, returns `exec()`.
  - `Application` wires modules in correct order and tears them down
    cleanly on quit.
- M: M1

### T-005 Cross-platform Paths + Logger
- D: T-004
- AC:
  - `Paths::dataDir()`, `downloadsDir()`, `logFile()`, `dbFile()`
    return correct per-OS locations using `QStandardPaths`.
  - `Logger` installs a `qInstallMessageHandler` writing to a rolling
    log file (`nodetalk.log`, max 1 MiB, 3 backups) and stderr.
- M: M1

### T-006 SQLite database & migrations
- D: T-005
- AC:
  - `Database::open()` opens the DB file, applies schema if
    `user_version == 0`, bumps version to current.
  - All five tables from PRD §8 exist.
  - Unit test `tst_database` verifies schema creation and idempotency.
- M: M1

### T-007 Repositories
- D: T-006
- AC:
  - `PeerRepository`, `MessageRepository`, `TransferRepository`,
    `EventRepository`, and `Settings` all expose typed CRUD APIs.
  - All queries use prepared statements and parameter binding.
  - Unit tests cover insert/update/list/find for each repository.
- M: M1

### T-008 Identity service
- D: T-007
- AC:
  - First run generates UUID + 32-byte random fingerprint, persisted
    via `Settings`.
  - Subsequent runs reload the same identity.
  - Unit test `tst_identity` verifies persistence.
- M: M1

### T-009 Wire protocol (framing + JSON envelopes)
- D: T-002
- AC:
  - `Protocol::encodeJson`, `encodeBinary`, `FrameReader::feed` round-
    trip cleanly on multi-frame buffers and short reads.
  - All message types from PRD §7.3 have typed C++ struct + (de)ser.
  - Malformed inputs are rejected without crashing.
  - Unit test `tst_protocol` exercises framing and all message types.
- M: M1

### T-010 Discovery service
- D: T-009, T-008
- AC:
  - Sends UDP broadcast + multicast `announce` every 5 s and on
    `start()`.
  - Emits `peerAnnounced(PeerInfo)` for received valid announces
    excluding self.
  - Sends `bye` on `stop()`.
- M: M1

### T-011 Transport server + PeerLink
- D: T-009
- AC:
  - `TransportServer` listens on `tcp_port` (auto if `0`).
  - Accepted sockets are wrapped in `PeerLink` which performs the
    `hello` / `hello_ack` handshake.
  - Outgoing connections are also `PeerLink` instances created by
    `PeerManager::connectTo(PeerInfo)`.
  - Disconnects emit `disconnected(reason)` and free resources.
- M: M1

### T-012 Peer manager
- D: T-010, T-011, T-007
- AC:
  - Reconciles discovery + DB into a unified `Peer` list.
  - Online state derived from active `PeerLink` and last-seen window.
  - Trust enforced before any non-`hello` traffic is delivered to UI.
  - Public API: `sendText`, `sendTyping`, `markRead`, `connectTo`,
    `trust`, `block`, `rename`.
- M: M1

### T-013 Main window shell + PeerList + ChatView + Input
- D: T-012
- AC:
  - One `QMainWindow` with a `QSplitter` (peer list left, chat right).
  - Peer list shows nickname, IP, online dot, offline = greyed.
  - Chat view shows scrollable history paged from the DB.
  - Input area sends text on Enter (Shift+Enter newline). Disabled
    if peer offline.
  - Typing indicator visible in chat header.
  - Status icons: sending / sent / delivered / read.
- M: M1

### T-014 Trust dialog
- D: T-013, T-012
- AC:
  - When `PeerManager` reports an unknown peer attempting `hello`,
    a non-modal trust prompt appears with peer nick, ip, fingerprint,
    `Trust` and `Reject` buttons. Decision is persisted.
- M: M1

### T-015 Manual add peer
- D: T-013
- AC:
  - `Peers → Add by IP…` dialog accepts IP + port; on confirm, opens
    a transient `PeerLink` to perform `hello`; result feeds normal
    trust flow.
- M: M1

### T-016 File transfer manager (basic)
- D: T-012, T-007
- AC:
  - Sender computes SHA-256, sends `file_offer`. Receiver-driven
    `file_chunk_req` loop streams chunks. `file_complete` finalizes.
  - Files appear as chat items in history.
  - Unit/integration test sends a multi-MB file between two
    in-process `PeerManager` instances.
- M: M1

### T-017 i18n bootstrap
- D: T-003, T-004
- AC:
  - English and Russian `.qm` files load; default = system locale,
    fallback English.
  - `lupdate` invocation captures all `tr()` strings.
- M: M1

### T-018 MVP smoke test
- D: T-013, T-016, T-017
- AC:
  - `tests/tst_smoke` boots the app in `offscreen` platform, opens
    main window, opens settings, exits with code 0.
- M: M1

---

## Milestone M2 — Final release
Goal: full feature set per PRD, polished UX, packaging, CI/CD.

### T-101 Tray icon + minimize-to-tray
- D: T-013
- AC:
  - `QSystemTrayIcon` with menu (Show/Hide, Quit). Close hides to tray
    when enabled. App keeps running in background.
- M: M2

### T-102 Desktop notifications
- D: T-101
- AC:
  - Tray balloon / native notification on new message and on
    file-related events when window is not active.
- M: M2

### T-103 Event log UI
- D: T-007
- AC:
  - Dockable bottom panel listing latest events with filter by kind
    and peer; backed by `EventRepository`.
- M: M2

### T-104 Transfers panel
- D: T-016
- AC:
  - Bottom-right tab listing all active and recent transfers with
    progress, speed, ETA, Pause / Resume / Cancel buttons.
- M: M2

### T-105 File transfer: pause / resume / cancel + queue
- D: T-016, T-104
- AC:
  - Receiver can pause; sender stops sending until resume.
  - On disconnect, transfer flips to `paused`, partial file kept;
    on reconnect with both peers online, user can resume.
  - Per-peer queue with default `max_parallel_transfers = 3`.
- M: M2

### T-106 File transfer: integrity validation
- D: T-016
- AC:
  - SHA-256 advertised in `file_offer` is verified by receiver after
    last chunk; mismatch → transfer marked failed and event logged.
- M: M2

### T-107 History search
- D: T-013, T-007
- AC:
  - Search bar above chat filters messages by substring within the
    open conversation; global search across peers from a separate
    dialog (`Ctrl+F`).
- M: M2

### T-108 Settings dialog
- D: T-013, T-017
- AC:
  - Tabs: General (nick, language, save folder), Network (ports,
    timeouts), Tray, Trusted Peers (rename, untrust, block).
  - All changes persisted; language change triggers immediate
    `retranslateUi` everywhere.
- M: M2

### T-109 Russian translation
- D: T-017
- AC:
  - All user-facing strings translated; missing-string check via
    `lupdate -no-obsolete` reports zero missing entries.
- M: M2

### T-110 Drag-and-drop file send
- D: T-016, T-013
- AC:
  - Dragging files onto chat input or chat view enqueues them as
    file offers to the active peer.
- M: M2

### T-111 Notifications + tray polish
- D: T-101, T-102
- AC:
  - Notifications respect "do not notify if active" rule and a
    Settings toggle.
- M: M2

### T-112 Logging + structured categories
- D: T-005
- AC:
  - `QLoggingCategory` per module: `nt.net`, `nt.disc`, `nt.db`,
    `nt.ui`, `nt.xfer`. `QT_LOGGING_RULES` honored.
- M: M2

### T-113 Packaging — Linux
- D: M1 complete
- AC:
  - CPack produces `.tar.gz` and `.deb`.
  - `packaging/linux/AppImage.sh` produces an AppImage from the install
    tree. `desktop` and `png` icons included.
- M: M2

### T-114 Packaging — Windows
- D: M1 complete
- AC:
  - CPack produces a `.zip` portable archive (with `windeployqt`).
  - `packaging/windows/nodetalk.iss` Inno Setup script produces the
    installer; CI step runs Inno Setup.
- M: M2

### T-115 Packaging — macOS
- D: M1 complete
- AC:
  - CMake bundles `NodeTalk.app` with `Info.plist`.
  - `macdeployqt` produces a self-contained bundle. CPack `.dmg` and
    plain `.zip` of the app are produced.
- M: M2

### T-116 GitHub Actions: CI workflow
- D: T-002, M1 complete
- AC:
  - `.github/workflows/ci.yml` builds & tests on
    `ubuntu-latest`, `windows-latest`, `macos-latest` for every push
    and PR. Uses Qt 6 from `jurplel/install-qt-action`.
- M: M2

### T-117 GitHub Actions: Release workflow
- D: T-113, T-114, T-115, T-116
- AC:
  - `.github/workflows/release.yml` triggers on tags `v*`.
  - Builds release artifacts on all three OSes.
  - Generates release notes from `git log <prev-tag>..<this-tag>`.
  - Uploads artifacts to the GitHub Release.
- M: M2

### T-118 Tests: integration & protocol
- D: T-009, T-016
- AC:
  - `tst_protocol`, `tst_database`, `tst_identity`, `tst_filetransfer`
    pass on all three CI runners.
- M: M2

### T-119 Documentation
- D: M1 complete
- AC:
  - `README.md` (overview, screenshots placeholder, install/build
    quick start).
  - `docs/ARCHITECTURE.md`, `PROTOCOL.md`, `BUILD.md`, `PACKAGING.md`,
    `RELEASE.md`, `TESTING.md`.
- M: M2

### T-120 Polish
- D: all of M2
- AC:
  - No `QTBUG`/warning floods during normal use.
  - `valgrind --leak-check=full` (Linux) reports no definite leaks
    after a 10-minute session.
  - All icons present, all dialogs sized reasonably, all dialogs
    respect tab order and keyboard navigation.
- M: M2

---

## Milestone M3 — UI Polish

Goal: lift the chat UI from "functional Qt Widgets" to a modern,
themeable, accessible messenger experience.

- [ ] **T-201 Bubble delegate for messages** — `ChatView` uses a custom
  `QStyledItemDelegate` that paints message bubbles sized to their
  content with rounded corners and a small tail. Bubbles never span
  the full row width; long messages wrap inside the bubble at ~70%
  of viewport width.
- [ ] **T-202 Day separators & author grouping** _(deps: T-201)_ —
  Messages are grouped by day with a centered separator
  ("Today" / "Yesterday" / localized date). Consecutive messages from
  the same author within 5 minutes share one header.
- [ ] **T-203 Avatars / colored initials** — `PeerListWidget` and
  `ChatView` show a circular avatar with the peer's initial(s);
  background color is derived from a hash of `peerId`.
- [ ] **T-204 Peer list polish** _(deps: T-203)_ — Online dot has a
  tooltip with `last_seen` and IP. Unread message badge per peer.
  Right-click context menu: rename alias, block, remove, copy
  fingerprint.
- [ ] **T-205 Status icons instead of glyph text** _(deps: T-201)_ —
  Message status (`Sending`/`Sent`/`Delivered`/`Read`/`Failed`)
  rendered as inline SVG icons inside the bubble; `Read` ticks
  colored with the palette accent.
- [ ] **T-206 Chat header bar** — Header above the chat view shows the
  peer avatar, display name, online state and IP. Header buttons:
  inline search (no modal), toggle Transfers dock, toggle Event log
  dock.
- [ ] **T-207 Clickable links in messages** _(deps: T-201)_ — URLs are
  auto-detected and rendered as clickable links that open in the
  default browser; text remains selectable; link color follows the
  palette accent.
- [ ] **T-208 Window-wide drag & drop overlay** — Dragging files
  anywhere over the main window shows a translucent overlay
  "Drop files to send to <peer>". Drop queues outgoing transfers; if
  no peer is selected a hint is shown instead of silently failing.
- [ ] **T-209 Empty states** — Empty peer list shows a centered hint
  "Add peer by IP… (Ctrl+N)" with a button. Empty chat shows
  "No messages yet — say hi!" with the peer avatar.
- [ ] **T-210 Application-wide style polish** — Single QSS resource
  (`resources/styles/nodetalk.qss`) loaded at startup; consistent
  spacing, radius, hover states. Transfers/Event log docks hidden
  by default, toggled via View menu.
- [ ] **T-211 HiDPI & themed icons** — All custom icons have light +
  dark variants picked based on the active palette; fall back to
  `QIcon::fromTheme` where a freedesktop name exists.
- [ ] **T-212 Smarter message input** — `MessageInput` auto-resizes
  from 1 line up to ~5 lines, then scrolls. Character counter
  appears near the protocol cap; over-cap input is blocked with a
  tooltip.
- [ ] **T-213 Tray notifications & unread badge** _(deps: T-204)_ —
  Receiving a message in a non-active chat shows a desktop
  notification (respecting Do-Not-Disturb). Tray icon shows a
  dot/number badge while unread messages exist.
- [ ] **T-214 Russian translation pass** — All M3 user-visible strings
  are wrapped in `tr()`; `resources/i18n/nodetalk_ru.ts` updated via
  `lupdate` and fully translated; runtime language switch still
  works.
