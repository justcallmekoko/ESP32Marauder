package com.marauder.controller.serial

import android.util.Log
import kotlinx.coroutines.*
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.collect
import java.net.Inet4Address
import java.net.NetworkInterface
import java.net.ServerSocket
import java.net.Socket

class TcpBridgeServer(private val repository: SerialRepository) {

    private val scope = CoroutineScope(Dispatchers.IO + SupervisorJob())
    private var serverSocket: ServerSocket? = null
    private var clientJob: Job? = null

    private val _status = MutableStateFlow("Bridge: stopped")
    val status: StateFlow<String> = _status.asStateFlow()

    // Bind to 0.0.0.0 (all interfaces) so Termux can connect via the phone's WiFi IP
    // (e.g. 192.168.1.5:7555). Traffic on wlan0 bypasses Android's per-UID iptables
    // rules that block loopback (127.0.0.1) connections between apps.
    fun start(port: Int = 7555) {
        scope.launch {
            try {
                val server = ServerSocket(port)  // binds to 0.0.0.0
                serverSocket = server
                val ip = wifiIp()
                Log.i(TAG, "TCP bridge listening on 0.0.0.0:$port (WiFi: $ip)")
                _status.value = "Bridge: $ip:$port"
                while (isActive) {
                    val client = try { server.accept() } catch (e: Exception) { break }
                    clientJob?.cancel()
                    clientJob = scope.launch { serveClient(client, port) }
                }
            } catch (e: Exception) {
                Log.e(TAG, "TCP bridge failed: ${e.message}", e)
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
        val remote = socket.inetAddress.hostAddress ?: "?"
        Log.i(TAG, "Client connected from $remote")
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
        _status.value = "Bridge: ${wifiIp()}:$port"
    }

    private fun wifiIp(): String {
        return try {
            NetworkInterface.getNetworkInterfaces()
                ?.asSequence()
                ?.filter { it.name.startsWith("wlan") || it.name.startsWith("ap") }
                ?.flatMap { it.inetAddresses.asSequence() }
                ?.filterIsInstance<Inet4Address>()
                ?.firstOrNull { !it.isLoopbackAddress }
                ?.hostAddress ?: "?.?.?.?"
        } catch (e: Exception) {
            "?.?.?.?"
        }
    }

    companion object {
        private const val TAG = "TcpBridgeServer"
    }
}
