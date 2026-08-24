import re
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


class SpiffsMachineProtocolTests(unittest.TestCase):
    def test_protocol_is_versioned_framed_and_transactional(self):
        header = (ROOT / "esp32_marauder" / "CommandLine.h").read_text()
        source = (ROOT / "esp32_marauder" / "CommandLine.cpp").read_text()

        self.assertIn('PROTOCOL_INFO_CMD[] = "protocolinfo"', header)
        self.assertIn('BACKUP_STATUS_CMD[] = "backupstatus"', header)
        self.assertIn('@MARAUDER:{\\"protocol\\":1', source)
        self.assertIn('this->argSearch(&cmd_args, "--machine")', source)
        self.assertIn('"INVALID_TRANSACTION"', source)
        self.assertIn('"SD_NOT_SUPPORTED"', source)
        self.assertRegex(source, r'machineResult\([^;]+"started"')
        self.assertRegex(source, r'machineResult\([^;]+"success"')

    def test_machine_restore_acknowledges_reboot(self):
        source = (ROOT / "esp32_marauder" / "CommandLine.cpp").read_text()
        restore = source[source.index("cmd_args.get(0) == RESTORE_SPIFFS_CMD) {") :]
        success = restore.index(
            'machineResult(transaction_id, command, "success", "OK", files, bytes, operation == 2)'
        )
        restart = restore.index("ESP.restart();")
        self.assertLess(success, restart)
        self.assertIn("if (success && operation == 2)", restore[success:restart])

    def test_backup_status_measures_the_activated_backup(self):
        source = (ROOT / "esp32_marauder" / "SDInterface.cpp").read_text()
        method = re.search(
            r"bool SDInterface::migrateSPIFFS\(.*?\n\}", source, re.S
        ).group(0)
        self.assertIn("File backup = SD.open(backup_path)", method)
        self.assertIn('copyTree(SD, backup_path, nullptr', method)


if __name__ == "__main__":
    unittest.main()
