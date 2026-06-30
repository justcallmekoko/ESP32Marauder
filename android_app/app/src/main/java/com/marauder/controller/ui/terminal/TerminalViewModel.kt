package com.marauder.controller.ui.terminal

import androidx.lifecycle.ViewModel
import androidx.lifecycle.ViewModelProvider
import androidx.lifecycle.viewModelScope
import com.marauder.controller.model.QuickAction
import com.marauder.controller.model.TerminalLine
import com.marauder.controller.serial.SerialRepository
import kotlinx.coroutines.flow.*
import kotlinx.coroutines.launch

class TerminalViewModel(private val repository: SerialRepository) : ViewModel() {

    private val _lines = MutableStateFlow<List<TerminalLine>>(emptyList())
    val lines: StateFlow<List<TerminalLine>> = _lines.asStateFlow()

    val isConnected: StateFlow<Boolean> = repository.isConnected
    val commandPending: StateFlow<Boolean> = repository.commandPending

    val quickActions = listOf(
        QuickAction("scanall",       "scanall"),
        QuickAction("stopscan",      "stopscan"),
        QuickAction("list -a",       "list -a"),
        QuickAction("list -c",       "list -c"),
        QuickAction("deauth",        "attack -t deauth"),
        QuickAction("settings",      "settings"),
        QuickAction("help",          "help"),
    )

    init {
        viewModelScope.launch {
            repository.outputFlow.collect { line ->
                val current = _lines.value
                _lines.value = if (current.size >= 2000) {
                    current.drop(1) + line
                } else {
                    current + line
                }
            }
        }
    }

    fun sendCommand(cmd: String) {
        if (cmd.isBlank()) return
        repository.send(cmd.trim())
    }

    fun clearTerminal() {
        _lines.value = emptyList()
    }

    class Factory(private val repository: SerialRepository) : ViewModelProvider.Factory {
        @Suppress("UNCHECKED_CAST")
        override fun <T : ViewModel> create(modelClass: Class<T>): T =
            TerminalViewModel(repository) as T
    }
}
