#  The MIT License
#
#  Copyright (C) 2025 Giuseppe Mastrangelo
#
#  Permission is hereby granted, free of charge, to any person obtaining
#  a copy of this software and associated documentation files (the
#  'Software'), to deal in the Software without restriction, including
#  without limitation the rights to use, copy, modify, merge, publish,
#  distribute, sublicense, and/or sell copies of the Software, and to
#  permit persons to whom the Software is furnished to do so, subject to
#  the following conditions:
#
#  The above copyright notice and this permission notice shall be
#   included in all copies or substantial portions of the Software.
#
#  THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND,
#  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
#  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
#  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
#  CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
#  TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
#  SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#
# -----------------------------------------------------------------------------
#
#  Contributors:
#    - Erdenebileg Byamba (https:#github.com/erd3n)
#        * Contribution: Initial implementation of SSE Transport protocol
#
# -----------------------------------------------------------------------------

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