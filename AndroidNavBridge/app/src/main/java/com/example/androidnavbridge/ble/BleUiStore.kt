package com.example.androidnavbridge.ble

import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.update

data class BleUiState(
    val status: String = "Disconnected",
    val lastWrite: String = "No BLE writes yet"
)

object BleUiStore {
    private val mutableState = MutableStateFlow(BleUiState())
    val state: StateFlow<BleUiState> = mutableState

    fun setStatus(status: String) {
        mutableState.update { it.copy(status = status) }
    }

    fun setLastWrite(lastWrite: String) {
        mutableState.update { it.copy(lastWrite = lastWrite) }
    }
}
