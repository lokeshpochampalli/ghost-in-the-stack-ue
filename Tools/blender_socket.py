import socket, json, sys
def send(cmd_type, params=None, timeout=60):
    s = socket.create_connection(("127.0.0.1", 9876), timeout=timeout)
    s.sendall(json.dumps({"type": cmd_type, "params": params or {}}).encode())
    chunks = []
    while True:
        try:
            c = s.recv(65536)
        except socket.timeout:
            break
        if not c: break
        chunks.append(c)
        try:
            json.loads(b"".join(chunks).decode()); break
        except Exception: continue
    s.close()
    return json.loads(b"".join(chunks).decode())
if __name__ == "__main__":
    cmd = sys.argv[1]
    params = json.loads(sys.argv[2]) if len(sys.argv) > 2 else {}
    if cmd == "execute_code" and params.get("file"):
        params = {"code": open(params["file"], encoding="utf-8").read()}
    print(json.dumps(send(cmd, params), indent=2)[:4000])
