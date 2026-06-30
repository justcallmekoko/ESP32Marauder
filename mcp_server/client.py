"""
ESP32 Marauder — Venice AI terminal client

Uses mcp-use to connect the Gemma 4 model on Venice AI to the Marauder
MCP server, giving it serial "hands" to control the hardware.

Setup:
    pip install mcp-use langchain-openai pyserial
    export VENICE_API_KEY="your_key_here"
    export MARAUDER_PORT="/dev/ttyUSB0"   # or leave unset for auto-detect
    python client.py
"""

import asyncio
import os
import sys
from pathlib import Path
from langchain_openai import ChatOpenAI
from mcp_use import MCPAgent, MCPClient

# ---------------------------------------------------------------------------
# Config
# ---------------------------------------------------------------------------

VENICE_BASE_URL = "https://api.venice.ai/api/v1"
DEFAULT_MODEL   = "gemma-4-uncensored"
SERVER_SCRIPT   = str(Path(__file__).parent / "server.py")

SYSTEM_PROMPT = """\
You are a hands-on ESP32 Marauder operator. You have direct serial access to
the hardware via MCP tools. When the user asks you to do something, use the
tools to actually do it — don't just describe how. Always connect first if not
already connected, then execute the requested operation and report real results.
"""


def build_mcp_config() -> dict:
    """Build the MCPClient config, forwarding the port env-var if set."""
    args = [sys.executable, SERVER_SCRIPT]
    port = os.getenv("MARAUDER_PORT")
    if port:
        args += ["--port", port]
    baud = os.getenv("MARAUDER_BAUD", "115200")
    args += ["--baud", baud]

    return {
        "mcpServers": {
            "esp32-marauder": {
                "command": args[0],
                "args": args[1:],
            }
        }
    }


# ---------------------------------------------------------------------------
# Main loop
# ---------------------------------------------------------------------------

async def main() -> None:
    api_key = os.getenv("VENICE_API_KEY")
    if not api_key:
        sys.exit("ERROR: VENICE_API_KEY environment variable is not set.")

    model_name = os.getenv("VENICE_MODEL", DEFAULT_MODEL)

    llm = ChatOpenAI(
        model=model_name,
        api_key=api_key,
        base_url=VENICE_BASE_URL,
        temperature=0.3,       # lower = more deterministic tool selection
        max_tokens=4096,
    )

    client = MCPClient.from_dict(build_mcp_config())

    agent = MCPAgent(
        llm=llm,
        client=client,
        max_steps=15,
        system_prompt=SYSTEM_PROMPT,
        verbose=False,         # set True to see every tool call
    )

    print(f"ESP32 Marauder AI Terminal")
    print(f"Model : {model_name} via Venice AI")
    print(f"Port  : {os.getenv('MARAUDER_PORT', 'auto-detect')}")
    print("Type 'quit' to exit, 'verbose' to toggle tool-call output.\n")

    verbose = False

    while True:
        try:
            line = input("\033[92m>\033[0m ").strip()
        except (EOFError, KeyboardInterrupt):
            print("\nBye.")
            break

        if not line:
            continue
        if line.lower() in ("quit", "exit", "q"):
            break
        if line.lower() == "verbose":
            verbose = not verbose
            agent.verbose = verbose
            print(f"Verbose {'on' if verbose else 'off'}.")
            continue

        try:
            result = await agent.run(line)
            print(f"\n{result}")
        except Exception as exc:
            print(f"\n[error] {exc}")

    await client.close_all_sessions()


if __name__ == "__main__":
    asyncio.run(main())
