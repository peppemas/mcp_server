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
from mcp import ClientSession, StdioServerParameters
from mcp.client.stdio import stdio_client
from contextlib import AsyncExitStack
from dotenv import load_dotenv
import json
import os

load_dotenv()

class MCPClient:
    def __init__(self):
        self.session: Optional[ClientSession] = None
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
        args = []
        env = {}
        for name, server in servers.items():
            command = server.get("command")
            args = server.get("args", [])
            env = server.get("env")

            #print(f"Server Name: {name}")
            #print(f"Command: {command}")
            #print(f"Envs: {env}")
            #print("Arguments:")
            #for arg in args:
            #    print(f"  {arg}")

        server_params = StdioServerParameters(
            command=command,
            args=args,
            env=env
        )

        stdio_transport = await self.exit_stack.enter_async_context(stdio_client(server_params))
        self.stdio, self.write = stdio_transport
        self.session = await self.exit_stack.enter_async_context(ClientSession(self.stdio, self.write))

        await self.session.initialize()

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
        print("Usage: python3 test-client.py <configuration.json>")
        sys.exit(1)

    client = MCPClient()
    try:
        await client.connect_to_server(sys.argv[1])
        await client.test()
    finally:
        await client.cleanup()

if __name__ == "__main__":
    import sys
    asyncio.run(main())
