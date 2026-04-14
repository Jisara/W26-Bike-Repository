package com.example.androidnavbridge.ble

import android.content.Context

object BleClientProvider {
    @Volatile
    private var instance: BleNusClient? = null

    fun get(context: Context): BleNusClient {
        return instance ?: synchronized(this) {
            instance ?: BleNusClient(context.applicationContext).also { instance = it }
        }
    }
}
