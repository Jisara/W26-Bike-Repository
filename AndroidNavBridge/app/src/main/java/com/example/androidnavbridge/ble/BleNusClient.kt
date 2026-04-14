package com.example.androidnavbridge.ble

import android.Manifest
import android.annotation.SuppressLint
import android.bluetooth.BluetoothAdapter
import android.bluetooth.BluetoothDevice
import android.bluetooth.BluetoothGatt
import android.bluetooth.BluetoothGattCallback
import android.bluetooth.BluetoothGattCharacteristic
import android.bluetooth.BluetoothGattService
import android.bluetooth.BluetoothManager
import android.bluetooth.BluetoothStatusCodes
import android.bluetooth.le.ScanFilter
import android.bluetooth.le.BluetoothLeScanner
import android.bluetooth.le.ScanSettings
import android.bluetooth.le.ScanCallback
import android.bluetooth.le.ScanResult
import android.content.Context
import android.content.pm.PackageManager
import android.os.Build
import android.os.Handler
import android.os.Looper
import android.os.ParcelUuid
import android.util.Log
import androidx.core.content.ContextCompat
import java.util.UUID

class BleNusClient(private val context: Context) {
    private val bluetoothManager: BluetoothManager =
        context.getSystemService(BluetoothManager::class.java)

    private val adapter: BluetoothAdapter?
        get() = bluetoothManager.adapter

    private val scanner: BluetoothLeScanner?
        get() = adapter?.bluetoothLeScanner

    private var gatt: BluetoothGatt? = null
    private var rxCharacteristic: BluetoothGattCharacteristic? = null
    private var scanInProgress = false
    private var pendingPayload: String? = null
    private val mainHandler = Handler(Looper.getMainLooper())
    private var negotiatedMtu: Int = 23
    private val targetMtu: Int = 185

    private val nusServiceUuid: UUID = UUID.fromString("6E400001-B5A3-F393-E0A9-E50E24DCCA9E")
    private val nusRxUuid: UUID = UUID.fromString("6E400002-B5A3-F393-E0A9-E50E24DCCA9E")

    private val scanTimeoutRunnable = Runnable {
        if (scanInProgress) {
            stopBleScan()
            scanInProgress = false
            BleUiStore.setStatus("Scan timed out (no Electrium-NAV found)")
        }
    }

    @SuppressLint("MissingPermission")
    fun connect() {
        if (!hasRuntimeBlePermission()) {
            BleUiStore.setStatus("Missing BLE runtime permissions")
            return
        }

        val localAdapter = adapter
        if (localAdapter == null || !localAdapter.isEnabled) {
            BleUiStore.setStatus("Bluetooth is disabled")
            return
        }

        val localScanner = scanner
        if (localScanner == null) {
            BleUiStore.setStatus("BLE scanner unavailable on this device")
            return
        }

        if (gatt != null && rxCharacteristic != null) {
            BleUiStore.setStatus("Already connected (NUS ready)")
            return
        }

        if (scanInProgress) {
            BleUiStore.setStatus("Scan already in progress...")
            return
        }
        BleUiStore.setStatus("Scanning for Electrium-NAV...")
        scanInProgress = true

        val filters = listOf(
            ScanFilter.Builder()
                .setServiceUuid(ParcelUuid(nusServiceUuid))
                .build()
        )
        val settings = ScanSettings.Builder()
            .setScanMode(ScanSettings.SCAN_MODE_LOW_LATENCY)
            .build()

        localScanner.startScan(filters, settings, scanCallback)
        mainHandler.removeCallbacks(scanTimeoutRunnable)
        mainHandler.postDelayed(scanTimeoutRunnable, 12000)
    }

    @SuppressLint("MissingPermission")
    fun disconnect() {
        mainHandler.removeCallbacks(scanTimeoutRunnable)
        stopBleScan()
        scanInProgress = false
        rxCharacteristic = null
        gatt?.disconnect()
        gatt?.close()
        gatt = null
        BleUiStore.setStatus("Disconnected")
    }

    @SuppressLint("MissingPermission")
    private fun stopBleScan() {
        try {
            scanner?.stopScan(scanCallback)
        } catch (e: Exception) {
            Log.e("BleNusClient", "Error stopping scan: ${e.message}")
        }
    }

    @SuppressLint("MissingPermission")
    fun sendText(text: String) {
        val payload = text.trim().take(180)
        if (payload.isEmpty()) {
            BleUiStore.setLastWrite("Skipped empty payload")
            return
        }

        val localGatt = gatt
        val characteristic = rxCharacteristic
        if (localGatt == null || characteristic == null) {
            pendingPayload = payload
            BleUiStore.setLastWrite("Queued while connecting: $payload")
            connect()
            return
        }

        writePayload(localGatt, characteristic, payload)
    }

    @SuppressLint("MissingPermission")
    private fun writePayload(
        localGatt: BluetoothGatt,
        characteristic: BluetoothGattCharacteristic,
        payload: String
    ) {
        val started = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            localGatt.writeCharacteristic(
                characteristic,
                payload.toByteArray(Charsets.UTF_8),
                BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT
            ) == BluetoothStatusCodes.SUCCESS
        } else {
            @Suppress("DEPRECATION")
            run {
                characteristic.writeType = BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT
                characteristic.value = payload.toByteArray(Charsets.UTF_8)
                localGatt.writeCharacteristic(characteristic)
            }
        }

        BleUiStore.setLastWrite(if (started) "Write started: $payload" else "Write failed to start")
        if (started) {
            pendingPayload = null
        }
    }

    private val scanCallback: ScanCallback = object : ScanCallback() {
        @SuppressLint("MissingPermission")
        override fun onScanResult(callbackType: Int, result: ScanResult) {
            val device: BluetoothDevice = result.device
            val hasNus = result.scanRecord?.serviceUuids?.any { it.uuid == nusServiceUuid } == true
            val hasName = device.name == "Electrium-NAV"
            if (!hasNus && !hasName) return

            mainHandler.removeCallbacks(scanTimeoutRunnable)
            stopBleScan()
            scanInProgress = false
            BleUiStore.setStatus("Found Electrium-NAV, connecting...")
            gatt?.close()
            gatt = device.connectGatt(context, false, gattCallback)
        }

        override fun onScanFailed(errorCode: Int) {
            mainHandler.removeCallbacks(scanTimeoutRunnable)
            scanInProgress = false
            BleUiStore.setStatus("BLE scan failed: $errorCode")
        }
    }

    private val gattCallback = object : BluetoothGattCallback() {
        @SuppressLint("MissingPermission")
        override fun onConnectionStateChange(gatt: BluetoothGatt, status: Int, newState: Int) {
            if (newState == BluetoothGatt.STATE_CONNECTED) {
                BleUiStore.setStatus("Connected, requesting MTU...")
                negotiatedMtu = 23
                val requested = gatt.requestMtu(targetMtu)
                if (!requested) {
                    BleUiStore.setStatus("Connected, MTU request failed; discovering services...")
                    gatt.discoverServices()
                }
            } else if (newState == BluetoothGatt.STATE_DISCONNECTED) {
                rxCharacteristic = null
                BleUiStore.setStatus("Disconnected (status=$status)")
                gatt.close()
                if (this@BleNusClient.gatt == gatt) {
                    this@BleNusClient.gatt = null
                }
                if (pendingPayload != null) {
                    connect()
                }
            }
        }

        @SuppressLint("MissingPermission")
        override fun onServicesDiscovered(gatt: BluetoothGatt, status: Int) {
            val service: BluetoothGattService? = gatt.getService(nusServiceUuid)
            val characteristic = service?.getCharacteristic(nusRxUuid)
            rxCharacteristic = characteristic
            if (characteristic != null) {
                BleUiStore.setStatus("Ready (NUS RX found)")
                val queued = pendingPayload
                if (!queued.isNullOrEmpty()) {
                    writePayload(gatt, characteristic, queued)
                }
            } else {
                BleUiStore.setStatus("Connected, NUS RX not found")
            }
        }

        override fun onMtuChanged(gatt: BluetoothGatt, mtu: Int, status: Int) {
            if (status == BluetoothGatt.GATT_SUCCESS) {
                negotiatedMtu = mtu
                BleUiStore.setStatus("MTU negotiated: $mtu, discovering services...")
            } else {
                negotiatedMtu = 23
                BleUiStore.setStatus("MTU request failed (status=$status), discovering services...")
            }
            gatt.discoverServices()
        }

        override fun onCharacteristicWrite(
            gatt: BluetoothGatt,
            characteristic: BluetoothGattCharacteristic,
            status: Int
        ) {
            if (status == BluetoothGatt.GATT_SUCCESS) {
                BleUiStore.setLastWrite("Write complete")
            } else {
                BleUiStore.setLastWrite("Write error: $status")
            }
        }
    }

    private fun hasRuntimeBlePermission(): Boolean {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.S) return true
        return ContextCompat.checkSelfPermission(
            context,
            Manifest.permission.BLUETOOTH_SCAN
        ) == PackageManager.PERMISSION_GRANTED &&
            ContextCompat.checkSelfPermission(
                context,
                Manifest.permission.BLUETOOTH_CONNECT
            ) == PackageManager.PERMISSION_GRANTED
    }
}
