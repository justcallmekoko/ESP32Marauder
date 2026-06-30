package com.marauder.controller.serial

/**
 * Stateful byte-stream parser that mirrors server.py's _read_until_prompt().
 * Accumulates incoming bytes, emits complete lines, and fires promptDetected
 * when the Marauder "> " prompt appears at the tail of the buffer.
 */
class PromptDetector {

    data class Result(val lines: List<String>, val promptDetected: Boolean)

    private val buf = StringBuilder()

    fun feed(bytes: ByteArray): Result {
        buf.append(bytes.toString(Charsets.UTF_8))
        return processBuffer()
    }

    private fun processBuffer(): Result {
        val lines = mutableListOf<String>()
        var promptDetected = false

        if (buf.length >= 2 && buf.endsWith("> ")) {
            // Strip the prompt and emit everything before it as lines
            val content = buf.substring(0, buf.length - 2)
            buf.clear()
            content.split('\n').forEach { line ->
                val trimmed = line.trimEnd('\r')
                if (trimmed.isNotEmpty()) lines.add(trimmed)
            }
            promptDetected = true
        } else {
            // Emit only complete newline-terminated lines; leave the rest in buf
            var idx: Int
            while (buf.indexOf('\n').also { idx = it } >= 0) {
                val line = buf.substring(0, idx).trimEnd('\r')
                buf.delete(0, idx + 1)
                if (line.isNotEmpty()) lines.add(line)
            }
        }

        return Result(lines, promptDetected)
    }

    fun reset() {
        buf.clear()
    }
}
