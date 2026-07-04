package com.marauder.controller.serial

import android.net.LocalServerSocket
import android.net.LocalSocket
import android.util.Log
import kotlinx.coroutines.*
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.collect

class TcpBridgeServer(private val repository: SerialRepository) {

    private val scope = CoroutineScope(Dispatchers.IO + SupervisorJob())
    private var serverSocket: LocalServerSocket? = null
    private var clientJob: Job? = null

    private val _status = MutableStateFlow("Bridge: stopped")
    val status: StateFlow<String> = _status.asStateFlow()

    // Abstract Unix domain socket — bypasses Android's inter-app TCP iptables isolation.
    // TCP loopback (127.0.0.1) is dropped by Android 12+ per-UID firewall rules.
    // Abstract sockets live in the Linux kernel socket namespace (no IP stack, no iptables).
    // Termux connects via: sock = socket.socket(AF_UNIX); sock.connect('\0marauder_bridge')
    fun start() {
        scope.launch {
            try {
                val server = LocalServerSocket(SOCKET_NAME)
                serverSocket = server
                Log.i(TAG, "Bridge listening on abstract socket \\0$SOCKET_NAME")
                _status.value = "Bridge: ready"
                while (isActive) {
                    val client = try { server.accept() } catch (e: Exception) { break }
                    clientJob?.cancel()
                    clientJob = scope.launch { serveClient(client) }
                }
            } catch (e: Exception) {
                Log.e(TAG, "Bridge failed: ${e.message}", e)
                _status.value = "Bridge ERROR: ${e.message}"
            }
        }
    }

    fun stop() {
        scope.cancel()
        runCatching { serverSocket?.close() }
        _status.value = "Bridge: stopped"
    }

    private suspend fun serveClient(socket: LocalSocket) = socket.use {
        _status.value = "Bridge: client connected"
        // USB → Unix socket: forward raw bytes from the serial read loop
        val outJob = scope.launch {
            repository.rawFlow.collect { chunk ->
                runCatching { socket.outputStream.write(chunk) }.onFailure { cancel() }
            }
        }
        // Unix socket → USB: forward bytes from the client to the serial port
        runCatching {
            val buf = ByteArray(256)
            while (currentCoroutineContext().isActive) {
                val n = withContext(Dispatchers.IO) { socket.inputStream.read(buf) }
                if (n <= 0) break
                repository.writeRaw(buf.copyOf(n))
            }
        }
        outJob.cancel()
        _status.value = "Bridge: ready"
    }

    companion object {
        private const val TAG = "TcpBridgeServer"
        const val SOCKET_NAME = "marauder_bridge"
    }
}
