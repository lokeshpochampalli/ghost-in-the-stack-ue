"""Run a Python file inside the running Unreal Editor via PythonScriptPlugin remote execution."""
import sys, time
sys.path.append(r"C:\Program Files\Epic Games\UE_5.8\Engine\Plugins\Experimental\PythonScriptPlugin\Content\Python")
import remote_execution as remote

def run_file(path, timeout=30):
    code = open(path, encoding="utf-8").read()
    rex = remote.RemoteExecution(); rex.start()
    node = None
    for _ in range(timeout):
        time.sleep(1)
        if rex.remote_nodes: node = rex.remote_nodes[0]; break
    if not node: rex.stop(); raise SystemExit("no Unreal remote-execution node found")
    rex.open_command_connection(node["node_id"])
    try:
        res = rex.run_command(code, unattended=True, exec_mode=remote.MODE_EXEC_FILE, raise_on_failure=False)
    finally:
        rex.close_command_connection(); rex.stop()
    print("SUCCESS:", res.get("success"))
    if not res.get("success"): print("RESULT/TRACEBACK:", str(res.get("result"))[:2000])
    for o in res.get("output", []):
        print(f"  [{o.get('type')}] {o.get('output')}".rstrip()[:400])
    return res

if __name__ == "__main__":
    run_file(sys.argv[1])
