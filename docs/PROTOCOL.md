# Wire protocol

NodeTalk uses two protocols on the local subnet:

* **Discovery** over UDP (broadcast + multicast).
* **Messaging / file transfer** over TCP, one connection per peer.

All payloads are UTF-8 JSON unless marked binary. All multi-byte
integers in framing headers are big-endian.

## 1. Discovery (UDP)

Two sockets: a broadcast socket on `255.255.255.255:45821` and a
multicast socket joined to `239.255.42.99:45822`. Every 5 s an
announcement is sent; on graceful shutdown a final `bye` is sent.

```json
{
  "type":      "announce",
  "proto":     1,
  "peer_id":   "550e8400-e29b-41d4-a716-446655440000",
  "nick":      "alice@laptop",
  "fp":        "deadbeef…",
  "tcp_port":  45823
}
```

A peer that has not been heard from in `peer_timeout` seconds (default
20) is considered offline. Announcements from `peer_id == own` are
discarded.

## 2. TCP framing

Every TCP frame:

```
+------------------+--------+----------------+
| u32 length (BE)  | u8 typ | payload bytes  |
+------------------+--------+----------------+
```

`length` covers `typ + payload` and must not exceed `1 MiB`. `typ`:

| Value | Meaning |
| --- | --- |
| `0x01` | JSON UTF-8 control message |
| `0x02` | Raw binary chunk (file transfer body) |

## 3. Handshake

1. Initiator opens TCP and sends `hello`.
2. Responder validates and sends `hello_ack`. Both sides transition to
   `Ready` when each has received the other's hello.
3. From this point any message type is allowed (subject to trust).

```json
{ "type": "hello",
  "proto": 1, "peer_id": "...", "fp": "...", "nick": "alice@laptop" }
```

```json
{ "type": "hello_ack",
  "proto": 1, "peer_id": "...", "fp": "...", "nick": "bob@desktop" }
```

If the peer is `Blocked`, the responder sends `bye{"reason":"blocked"}`
and closes. If the peer is `Unknown` or its fingerprint changed, the
local side fires a `trustPromptRequested` signal and stops processing
non-handshake traffic until the user decides.

## 4. Chat

```json
{ "type": "text", "msg_id": "...", "ts": 1700000000, "body": "hi" }
```

Receiver sends back:

```json
{ "type": "text_ack", "msg_id": "...", "status": "delivered" }
{ "type": "text_ack", "msg_id": "...", "status": "read" }
```

Typing indicator (debounced):

```json
{ "type": "typing", "active": true }
```

## 5. File transfer

The protocol is **receiver-driven**: the sender never pushes a chunk
that the receiver did not request, which makes pause / resume / replay
trivial.

### 5.1 Offer

```json
{ "type": "file_offer",
  "xfer_id":   "uuid",
  "name":      "report.pdf",
  "size":      4823917,
  "mime":      "application/pdf",
  "sha256":    "…",
  "chunk_size":65536 }
```

### 5.2 Accept / reject

```json
{ "type": "file_accept", "xfer_id": "...", "resume_offset": 0 }
{ "type": "file_reject", "xfer_id": "...", "reason": "no space" }
```

`resume_offset` lets the receiver resume after a crash; the sender will
start from the chunk index containing that byte.

### 5.3 Chunk loop

Receiver, in a loop until done:

```json
{ "type": "file_chunk_req", "xfer_id": "...", "index": 12 }
```

Sender replies with two consecutive frames:

```json
{ "type": "file_chunk_meta", "xfer_id": "...", "index": 12, "size": 65536 }
```

…immediately followed by a `0x02` binary frame whose payload is the
chunk body (`size` bytes).

### 5.4 Pause / resume / cancel

```json
{ "type": "file_pause",  "xfer_id": "..." }
{ "type": "file_resume", "xfer_id": "..." }
{ "type": "file_complete", "xfer_id": "...", "ok": true }
```

When the receiver pauses it stops sending `file_chunk_req`. To resume
it sends `file_resume` and continues the loop. On completion the
receiver verifies SHA-256 against the offer and sends `file_complete`
with `ok` reflecting the result.

## 6. Bye

```json
{ "type": "bye", "reason": "shutdown" }
```

Either side may close immediately after.

## 7. Limits

* Max frame size: 1 MiB.
* Max concurrent transfers per peer: configurable, default 3.
* Default chunk size: 64 KiB.
