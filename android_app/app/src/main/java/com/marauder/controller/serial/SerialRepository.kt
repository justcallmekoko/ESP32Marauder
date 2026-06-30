package com.marauder.controller.serial

import android.hardware.usb.UsbDeviceConnection
import com.hoho.android.usbserial.driver.UsbSerialPort
import com.marauder.controller.model.LineType
import com.marauder.controller.model.TerminalLine
import kotlinx.coroutines.*
import kotlinx.coroutines.flow.*
import java.io.IOException

class SerialRepository {

    private val _outputFlow = MutableSharedFlow<TerminalLine>(extraBufferCapacity = 512)
    val outputFlow: SharedFlow<TerminalLine> = _outputFlow.asSharedFlow()

    private val _commandPending = MutableStateFlow(false)
    val commandPending: StateFlow<Boolean> = _commandPending.asStateFlow()

    private val _isConnected = MutableStateFlow(false)
    val isConnected: StateFlow<Boolean> = _isConnected.asStateFlow()

    private val scope = CoroutineScope(Dispatchers.IO + SupervisorJob())
    private val detector = PromptDetector()

    @Volatile private var port: UsbSerialPort? = null
    private var readJob: Job? = null

    fun connect(usbPort: UsbSerialPort, usbConnection: UsbDeviceConnection) {
        scope.launch {
            try {
                usbPort.open(usbConnection)
                usbPort.setParameters(115200, 8, UsbSerialPort.STOPBITS_1, UsbSerialPort.PARITY_NONE)
                // Drain boot banner (matches server.py's time.sleep(0.5) + reset_input_buffer)
                val dumpBuf = ByteArray(4096)
                try { usbPort.read(dumpBuf, 600) } catch (_: Exception) {}
                port = usbPort
                detector.reset()
                _commandPending.value = false
                _isConnected.value = true
                emit(TerminalLine("Connected at 115200 baud", LineType.SYSTEM))
                startReadLoop()
            } catch (e: Exception) {
                emit(TerminalLine("Connection error: ${e.message}", LineType.SYSTEM))
            }
        }
    }

    fun disconnect() {
        readJob?.cancel()
        readJob = null
        val p = port
        port = null
        try { p?.close() } catch (_: IOException) {}
        _isConnected.value = false
        _commandPending.value = false
        detector.reset()
        emit(TerminalLine("Disconnected", LineType.SYSTEM))
    }

    fun send(command: String) {
        val p = port ?: return
        _commandPending.value = true
        emit(TerminalLine(command, LineType.SENT))
        scope.launch {
            try {
                p.write(("$command\n").toByteArray(Charsets.UTF_8), 2000)
            } catch (e: IOException) {
                _commandPending.value = false
                emit(TerminalLine("Send error: ${e.message}", LineType.SYSTEM))
            }
        }
    }

    fun close() {
        disconnect()
        scope.cancel()
    }

    private fun startReadLoop() {
        readJob = scope.launch {
            val buf = ByteArray(4096)
            while (isActive) {
                val p = port ?: break
                try {
                    val n = p.read(buf, 50)
                    if (n > 0) {
                        val result = detector.feed(buf.copyOf(n))
                        for (line in result.lines) {
                            emit(TerminalLine(line, LineType.RECEIVED))
                        }
                        if (result.promptDetected) {
                            _commandPending.value = false
                        }
                    }
                } catch (e: IOException) {
                    if (isActive) {
                        emit(TerminalLine("Connection lost: ${e.message}", LineType.SYSTEM))
                        _isConnected.value = false
                        _commandPending.value = false
                    }
                    break
                }
            }
        }
    }

    private fun emit(line: TerminalLine) {
        scope.launch { _outputFlow.emit(line) }
    }
}
