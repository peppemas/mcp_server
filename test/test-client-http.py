import requests
import json
import threading
import time

def listen_sse():
    response = requests.get('http://127.0.0.1:8080/mcp', stream=True)
    for line in response.iter_lines(chunk_size=128):
        print(f"SSE: {line}")

# Launch in background SSE listener
threading.Thread(target=listen_sse, daemon=True).start()
time.sleep(1)

# Send initialization
init_msg = {
    "jsonrpc": "2.0",
    "id": 1,
    "method": "initialize",
    "params": {
        "protocolVersion": "2024-11-05",
        "capabilities": {},
        "clientInfo": {"name": "test-client", "version": "0.7.0"}
    }
}

response = requests.post('http://127.0.0.1:8080/mcp', 
                        json=init_msg,
                        headers={'Content-Type': 'application/json'})

print(f"Init: {response.json()}")

# Inited notification
init_notif = {
  "jsonrpc": "2.0",
  "method": "notifications/initialized"
}

response = requests.post('http://127.0.0.1:8080/mcp',
                        json=init_notif, 
                        headers={'Content-Type': 'application/json'})

print(f"Inited Notif: {response.json()}")

# List tools
tools_msg = {
    "jsonrpc": "2.0", 
    "id": 2,
    "method": "tools/list"
}

response = requests.post('http://localhost:8080/mcp',
                        json=tools_msg, 
                        headers={'Content-Type': 'application/json'})
print(f"Tools: {response.json()}")

# List tools
resources_msg = {
    "jsonrpc": "2.0", 
    "id": 2,
    "method": "resources/list"
}

response = requests.post('http://localhost:8080/mcp',
                        json=resources_msg, 
                        headers={'Content-Type': 'application/json'})
print(f"Resources: {response.json()}")


# Inited notification
prompts_msg = {
  "jsonrpc": "2.0",
  "method": "prompts/list"
}

response = requests.post('http://localhost:8080/mcp',
                        json=prompts_msg, 
                        headers={'Content-Type': 'application/json'})

time.sleep(5)  # Keep alive to see SSE messages