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
You are a hands-on ESP32 Marauder RF-security analyst with direct USB serial
access to the hardware via MCP tools.

## Workflow
1. If not connected, call connect() first. It auto-disables SD-card capture so
   ALL scan/sniff data streams back to this Linux host through USB serial.
2. To gather data, use scan_and_capture(scan_type, duration) — it starts the
   scan, collects live serial output for `duration` seconds, stops the scan,
   then pulls the AP/station/SSID lists. Everything is buffered locally.
3. The captured data is returned directly and also stored in the capture buffer.
   Call get_capture() at any time to re-read the last capture without re-scanning.
4. To persist findings, call save_capture_local() — it writes both a .txt
   (human-readable) and a .json (structured) file to ~/marauder_captures/ on
   this host.

## Serial output line formats (for parsing capture data)
- scanall AP discovery:    `<rssi> Ch: <ch> <bssid> ESSID: <ssid> <cap_hex...>`
  - Hidden AP (empty SSID): ESSID field contains the BSSID again instead of a name
  - Only fires ONCE per AP (first discovery); duplicates are suppressed
- scanall station:         `<n>: ap: <bssid> -> sta: <mac>`  (or sta -> ap)
- beacon sniff:            same format as scanall AP but fires for EVERY beacon (~10/s per AP)
- deauth sniff:            `<rssi> Ch: <ch> <src_mac> -> <dst_mac>`
- pmkid sniff:             `Received EAPOL: <bssid>` + periodic stats block every ~1 s
- raw sniff:               NO per-packet text; only periodic stats every ~1 s (use for volume metrics)
- bt/airtag sniff:         `<rssi> Device: <name_or_mac>` (one per advertisement, already newline-split)
- Periodic stats block (raw/pmkid): multi-line block with Mgmt/Data/Channel/Beacon/Probe/RSSI counts

## Analysis guidance
- Parse beacon captures: count unique BSSIDs, spot channels, note open (no cap_hex 0x11) vs encrypted.
- Deauth floods: many lines from the same src_mac to ff:ff:ff:ff:ff:ff = broadcast deauth attack.
- Hidden SSIDs: ESSID field equals BSSID string = AP is hiding its name.
- Rogue AP signals: same SSID on multiple BSSIDs, or BSSID on an unexpected channel.
- PMKID/EAPOL: count "Received EAPOL" lines; "Complete EAPOL: N" in stats = full 4-way handshake.
- BT/AirTag: group by RSSI range to estimate proximity; repeated MAC = persistent tracker.

When the user asks you to do something, use the tools to actually do it — do not
just describe how. Report real numbers and specific findings from the capture
data, not generic advice.
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

    timeout = int(os.getenv("VENICE_TIMEOUT", "120"))

    llm = ChatOpenAI(
        model=model_name,
        api_key=api_key,
        base_url=VENICE_BASE_URL,
        temperature=0.3,       # lower = more deterministic tool selection
        max_tokens=4096,
        timeout=timeout,       # fail fast if Venice AI hangs; override with VENICE_TIMEOUT
        streaming=True,        # stream tokens so the connection stays alive
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
        except asyncio.CancelledError:
            print("\n[cancelled]")
        except KeyboardInterrupt:
            print("\n[cancelled]")
        except Exception as exc:
            print(f"\n[error] {exc}")

    await client.close_all_sessions()


if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("\nBye.")
