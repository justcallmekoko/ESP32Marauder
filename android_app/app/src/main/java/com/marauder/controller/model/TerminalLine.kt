package com.marauder.controller.model

enum class LineType { SENT, RECEIVED, SYSTEM }

data class TerminalLine(
    val text: String,
    val type: LineType,
)
