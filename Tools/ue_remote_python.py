"""Run a Python file inside the running Unreal Editor via the Python plugin's remote execution.

    python Tools/ue_remote_python.py path/to/script.py

Needs bRemoteExecution=True (committed in Config/DefaultEngine.ini) and the editor open. Uses
Epic's own client, Engine/Plugins/Experimental/PythonScriptPlugin/Content/Python/remote_execution.py.
ExecuteFile mode takes a file path, exactly like the editor console's `py <file>`.
"""
import os
import sys
import time

ENGINE_PY = r"C:\Program Files\Epic Games\UE_5.8\Engine\Plugins\Experimental\PythonScriptPlugin\Content\Python"
sys.path.append(ENGINE_PY)
import remote_execution as remote  # noqa: E402


def run_file(path, timeout=30):
    path = os.path.abspath(path)
    if not os.path.isfile(path):
        raise SystemExit(f"no such file: {path}")
    rex = remote.RemoteExecution()
    rex.start()
    node = None
    for _ in range(timeout):
        time.sleep(1)
        if rex.remote_nodes:
            node = rex.remote_nodes[0]
            break
    if not node:
        rex.stop()
        raise SystemExit("no Unreal remote-execution node found; is the editor running with bRemoteExecution=True?")
    rex.open_command_connection(node["node_id"])
    try:
        res = rex.run_command(path, unattended=True, exec_mode=remote.MODE_EXEC_FILE, raise_on_failure=False)
    finally:
        rex.close_command_connection()
        rex.stop()
    print("SUCCESS:", res.get("success"))
    if not res.get("success"):
        print("RESULT/TRACEBACK:", str(res.get("result"))[:2000])
    for o in res.get("output", []):
        print(f"  [{o.get('type')}] {o.get('output')}".rstrip()[:400])
    return res


if __name__ == "__main__":
    if len(sys.argv) < 2:
        raise SystemExit(__doc__)
    run_file(sys.argv[1])
