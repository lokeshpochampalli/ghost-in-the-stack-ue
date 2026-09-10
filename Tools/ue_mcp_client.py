"""Minimal MCP streamable-HTTP client for the Unreal MCP server."""
import json, sys, urllib.request, urllib.error
URL = "http://127.0.0.1:8000/mcp"
HDR = {"Content-Type": "application/json", "Accept": "application/json, text/event-stream"}
_session = None
_id = [0]

def _post(payload, notify=False):
    global _session
    h = dict(HDR)
    if _session: h["Mcp-Session-Id"] = _session
    req = urllib.request.Request(URL, data=json.dumps(payload).encode(), headers=h, method="POST")
    try:
        r = urllib.request.urlopen(req, timeout=300)
    except urllib.error.HTTPError as e:
        return {"http_error": e.code, "body": e.read().decode(errors="replace")[:2000]}
    sid = r.headers.get("Mcp-Session-Id")
    if sid: _session = sid
    body = r.read().decode(errors="replace")
    if notify or not body.strip(): return None
    ctype = r.headers.get("Content-Type", "")
    if "text/event-stream" in ctype:
        msgs = []
        for line in body.splitlines():
            if line.startswith("data:"):
                try: msgs.append(json.loads(line[5:].strip()))
                except Exception: pass
        for m in msgs:
            if "result" in m or "error" in m: return m
        return msgs[-1] if msgs else None
    return json.loads(body)

def call(method, params=None):
    _id[0] += 1
    return _post({"jsonrpc": "2.0", "id": _id[0], "method": method, "params": params or {}})

def init():
    r = call("initialize", {"protocolVersion": "2025-03-26", "capabilities": {}, "clientInfo": {"name": "gits-smoke", "version": "0.1"}})
    _post({"jsonrpc": "2.0", "method": "notifications/initialized"}, notify=True)
    return r

def tool(name, args=None):
    return call("tools/call", {"name": name, "arguments": args or {}})

def text_of(resp):
    try:
        return "\n".join(c.get("text", "") for c in resp["result"]["content"] if c.get("type") == "text")
    except Exception:
        return json.dumps(resp, indent=2)

if __name__ == "__main__":
    print(json.dumps(init(), indent=2)[:1500])
    tl = call("tools/list")
    print("TOOLS:", [t["name"] for t in tl.get("result", {}).get("tools", [])])
