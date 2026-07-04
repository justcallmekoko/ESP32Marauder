package com.marauder.controller.serial

import kotlinx.coroutines.*
import kotlinx.coroutines.flow.collect
import java.net.InetAddress
import java.net.ServerSocket
import java.net.Socket

class TcpBridgeServer(private val repository: SerialRepository) {

    private val scope = CoroutineScope(Dispatchers.IO + SupervisorJob())
    private var serverSocket: ServerSocket? = null
    private var clientJob: Job? = null

    fun start(port: Int = 5555) {
        scope.launch {
            runCatching {
                val server = ServerSocket(port, 1, InetAddress.getLoopbackAddress())
                serverSocket = server
                while (isActive) {
                    val client = runCatching { server.accept() }.getOrNull() ?: break
                    clientJob?.cancel()
                    clientJob = scope.launch { serveClient(client) }
                }
            }
        }
    }

    fun stop() {
        scope.cancel()
        runCatching { serverSocket?.close() }
    }

    private suspend fun serveClient(socket: Socket) = socket.use {
        // USB → TCP: forward raw bytes from the serial read loop to the TCP client
        val outJob = scope.launch {
            repository.rawFlow.collect { chunk ->
                runCatching { socket.outputStream.write(chunk) }.onFailure { cancel() }
            }
        }
        // TCP → USB: forward bytes from the TCP client to the serial port
        runCatching {
            val buf = ByteArray(256)
            while (isActive && !socket.isClosed) {
                val n = withContext(Dispatchers.IO) { socket.inputStream.read(buf) }
                if (n <= 0) break
                repository.writeRaw(buf.copyOf(n))
            }
        }
        outJob.cancel()
    }
}
