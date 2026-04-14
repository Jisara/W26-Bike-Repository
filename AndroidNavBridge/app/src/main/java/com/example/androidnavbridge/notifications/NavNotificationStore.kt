package com.example.androidnavbridge.notifications

import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.update

data class NavNotificationState(
    val navText: String = "No nav text captured yet",
    val lastSource: String = "None"
)

object NavNotificationStore {
    private val mutableState = MutableStateFlow(NavNotificationState())
    val state: StateFlow<NavNotificationState> = mutableState

    fun updateFromMaps(text: String): String? {
        val sanitized = text.replace(Regex("\\s+"), " ").trim()
        if (sanitized.isEmpty()) return null
        if (sanitized != mutableState.value.navText) {
            mutableState.update {
                it.copy(navText = sanitized, lastSource = "Google Maps notification")
            }
        }
        return sanitized
    }
}
