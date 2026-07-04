package com.marauder.controller.serial

import android.util.Log
import kotlinx.coroutines.*
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.collect
import java.net.InetAddress
import java.net.ServerSocket
import java.net.Socket

class TcpBridgeServer(private val repository: SerialRepository) {

    private val scope = CoroutineScope(Dispatchers.IO + SupervisorJob())
    private var serverSocket: ServerSocket? = null
    private var clientJob: Job? = null

    private val _status = MutableStateFlow("Bridge: stopped")
    val status: StateFlow<String> = _status.asStateFlow()

    // Port 7555 — avoids the ADB default port (5555) which adbd owns on developer phones.
    // Explicit IPv4 bind prevents IPv6/IPv4 mismatch when getLoopbackAddress() returns ::1.
    fun start(port: Int = 7555) {
        scope.launch {
            try {
                val server = ServerSocket(port, 1, InetAddress.getByName("127.0.0.1"))
                serverSocket = server
                Log.i(TAG, "TCP bridge listening on 127.0.0.1:$port")
                _status.value = "Bridge: listening on :$port"
                while (isActive) {
                    val client = try { server.accept() } catch (e: Exception) { break }
                    clientJob?.cancel()
                    clientJob = scope.launch { serveClient(client, port) }
                }
            } catch (e: Exception) {
                Log.e(TAG, "TCP bridge failed to start: ${e.message}", e)
                _status.value = "Bridge ERROR: ${e.message}"
            }
        }
    }

    fun stop() {
        scope.cancel()
        runCatching { serverSocket?.close() }
        _status.value = "Bridge: stopped"
    }

    private suspend fun serveClient(socket: Socket, port: Int) = socket.use {
        _status.value = "Bridge: client connected"
        // USB → TCP: forward raw bytes from the serial read loop to the TCP client
        val outJob = scope.launch {
            repository.rawFlow.collect { chunk ->
                runCatching { socket.outputStream.write(chunk) }.onFailure { cancel() }
            }
        }
        // TCP → USB: forward bytes from the TCP client to the serial port
        runCatching {
            val buf = ByteArray(256)
            while (currentCoroutineContext().isActive && !socket.isClosed) {
                val n = withContext(Dispatchers.IO) { socket.inputStream.read(buf) }
                if (n <= 0) break
                repository.writeRaw(buf.copyOf(n))
            }
        }
        outJob.cancel()
        _status.value = "Bridge: listening on :$port"
    }

    companion object {
        private const val TAG = "TcpBridgeServer"
    }
}
