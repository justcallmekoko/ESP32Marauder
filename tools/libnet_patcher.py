#
# libnet80211.a patcher
#
# Written by Google Ai & Anton Ermakov, 2026
#

# Replace this paths by your own.
file_path = "/home/anton/Arduino/hardware/espressif/arduino-esp32/tools/esp32-arduino-libs/esp32c5/lib/libnet80211.a"

# Find the beginning of the function
# riscv32-esp-elf-objdump -d libnet80211.a | grep -A 15 "<ieee80211_raw_frame_sanity_check>"
#
# The beginning looks like this (but check with objdump!):
target = b"\x79\x71\x06\xd6\x22\xd4\x26\xd2\x4a\xd0\x4e\xce\x52\xcc\x56\xca\x5a\xc8\x95\xe9"

# Can be replaced with: li a0, 1 ​​(05 45) + ret (67 80) - always return 0x1
# Now change to: li a0, 0 (01 45) + ret (67 80) - always return 0x0
# The logic of this function has changed in the new library. Now, if the check 
# is successful, it returns 0x0, and if it fails, it returns some number != 0.
# So, we return 0x0 here.
replacement = b"\x01\x45\x82\x80\x01\x00\x01\x00\x01\x00\x01\x00\x01\x00\x01\x00\x01\x00\x01\x00"

with open(file_path, "rb") as f:
    data = f.read()

count = data.count(target)
if(count != 1):
    print(f"Found {count} signatures. Patch is not applied!")
    exit()

if target in data:
    new_data = data.replace(target, replacement + target[len(replacement):], 1)
    with open(file_path, "wb") as f:
        f.write(new_data)
    print("Patch successully applied!")
