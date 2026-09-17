Import("env")

from pathlib import Path

pkg = env.PioPlatform().get_package_dir("framework-arduinoespressif32")
if not pkg:
    print("[preprocess] framework-arduinoespressif32 not found")
else:
    network_src = Path(pkg) / "libraries" / "Network" / "src"
    if network_src.exists():
        env.Append(CPPPATH=[str(network_src)])
        print(f"[preprocess] Network include: {network_src}")
    else:
        print(f"[preprocess] Network/src missing under {pkg}")
