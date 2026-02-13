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
#  included in all copies or substantial portions of the Software.
#
#  THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND,
#  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
#  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
#  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
#  CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
#  TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
#  SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

import asyncio
import json
import os
import sys
from typing import Optional
from mcp import ClientSession
from mcp.client.session_group import ClientSessionGroup, StreamableHttpParameters
from contextlib import AsyncExitStack
from dotenv import load_dotenv

load_dotenv()

class MCPClient:
    def __init__(self):
        self.session: Optional[ClientSession] = None
        self.session_group: Optional[ClientSessionGroup] = None
        self.exit_stack = AsyncExitStack()

    async def connect_to_server(self, json_file_path: str):
        if not os.path.exists(json_file_path):
            print(f"File not found: {json_file_path}")
            exit(1)

        # Load and parse the JSON file
        with open(json_file_path, 'r') as file:
            try:
                data = json.load(file)
            except json.JSONDecodeError as e:
                print(f"Error decoding JSON: {e}")
                exit(1)

        # Access and interpret the data
        servers = data.get("mcpServers", {})
        server_url = None
        for name, server in servers.items():
            server_url = server.get("url")
            break

        print(f"Connecting to URL: {server_url}")

        try:
            print("Creating session group...")
            self.session_group = await self.exit_stack.enter_async_context(ClientSessionGroup())

            server_params = StreamableHttpParameters(
                url=server_url
            )

            print("Connecting to HTTP Stream server...")
            self.session = await self.session_group.connect_to_server(server_params)
            print("Connected and session initialized successfully")

        except Exception as e:
            print(f"Connection failed with error: {e}")
            import traceback
            traceback.print_exc()
            raise

    async def test(self):
        tool_results = []

        response = await self.session.list_tools()
        tools = response.tools
        print("Available tools:", [tool.name for tool in tools])

        response = await self.session.list_resources()
        resources = response.resources
        print("Available resources:", [resource.name for resource in resources])

        response = await self.session.list_prompts()
        prompts = response.prompts
        print("Available prompts:", [prompt.name for prompt in prompts])

        # Execute tool call [sleep]
        tool_name = "sleep"
        tool_args = { "milliseconds": 1000 }
        result = await self.session.call_tool(tool_name, tool_args)
        tool_results.append({"call": tool_name, "result": result})

        # Execute tool call [weather]
        tool_name = "get_weather"
        tool_args = { "city": "milano", "latitude": "45.464664", "longitude": "9.188540" }
        result = await self.session.call_tool(tool_name, tool_args)
        tool_results.append({"call": tool_name, "result": result})

        # Execute tool call [logging_test]
        tool_name = "logging_test"
        tool_args = {}
        result = await self.session.call_tool(tool_name, tool_args)
        tool_results.append({"call": tool_name, "result": result})

        print("================ RESULTS ===================")
        for result in tool_results:
            print(result)
            print()

    async def cleanup(self):
        await self.exit_stack.aclose()

async def main():
    if len(sys.argv) < 2:
        print("Usage: python3 test-client-http.py <configuration.json>")
        sys.exit(1)

    client = MCPClient()
    try:
        await client.connect_to_server(sys.argv[1])
        await client.test()
    finally:
        await client.cleanup()

if __name__ == "__main__":
    asyncio.run(main())
