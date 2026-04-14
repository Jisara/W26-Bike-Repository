package com.example.androidnavbridge

import android.Manifest
import android.os.Bundle
import android.provider.Settings
import android.content.Intent
import android.os.Build
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.Button
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import androidx.core.content.ContextCompat
import android.content.pm.PackageManager
import com.example.androidnavbridge.ble.BleClientProvider
import com.example.androidnavbridge.ble.BleUiStore
import com.example.androidnavbridge.notifications.NavNotificationStore
import com.example.androidnavbridge.ui.theme.AndroidNavBridgeTheme

class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()
        setContent {
            AndroidNavBridgeTheme {
                Scaffold(modifier = Modifier.fillMaxSize()) { innerPadding ->
                    PhaseOneScreen(
                        modifier = Modifier.padding(innerPadding)
                    )
                }
            }
        }
    }
}

@Composable
fun PhaseOneScreen(modifier: Modifier = Modifier) {
    val context = LocalContext.current
    val notificationState by NavNotificationStore.state.collectAsState()
    val bleState by BleUiStore.state.collectAsState()
    val bleClient = BleClientProvider.get(context.applicationContext)

    val requestBlePermissions = rememberLauncherForActivityResult(
        ActivityResultContracts.RequestMultiplePermissions()
    ) { result ->
        val grantedAll = result.values.all { it }
        if (grantedAll) {
            bleClient.connect()
        } else {
            BleUiStore.setStatus("BLE permissions denied")
        }
    }

    LaunchedEffect(Unit) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            val hasScan = ContextCompat.checkSelfPermission(
                context,
                Manifest.permission.BLUETOOTH_SCAN
            ) == PackageManager.PERMISSION_GRANTED
            val hasConnect = ContextCompat.checkSelfPermission(
                context,
                Manifest.permission.BLUETOOTH_CONNECT
            ) == PackageManager.PERMISSION_GRANTED

            if (hasScan && hasConnect) {
                bleClient.connect()
            } else {
                requestBlePermissions.launch(
                    arrayOf(
                        Manifest.permission.BLUETOOTH_SCAN,
                        Manifest.permission.BLUETOOTH_CONNECT
                    )
                )
            }
        } else {
            bleClient.connect()
        }
    }

    Column(
        modifier = modifier
            .fillMaxSize()
            .padding(16.dp),
        verticalArrangement = Arrangement.spacedBy(12.dp)
    ) {
        Text(text = "Electrium Nav Bridge")
        Text(text = "Automatic mode: Maps notification -> BLE -> ESP32")
        Text(text = "Nav Text: ${notificationState.navText}")
        Text(text = "Source: ${notificationState.lastSource}")
        Text(text = "BLE Status: ${bleState.status}")
        Text(text = "BLE Last Write: ${bleState.lastWrite}")

        Button(
            onClick = { bleClient.connect() },
            modifier = Modifier.fillMaxWidth()
        ) {
            Text("Connect BLE Now")
        }

        Button(
            onClick = { bleClient.disconnect() },
            modifier = Modifier.fillMaxWidth()
        ) {
            Text("Disconnect BLE")
        }

        Button(
            onClick = {
                bleClient.sendText("In 200 m, turn right onto Main Street. 2.1 km, 6 min")
            },
            modifier = Modifier.fillMaxWidth()
        ) {
            Text("Send Test Directions")
        }

        Button(
            onClick = {
                bleClient.sendText(notificationState.navText)
            },
            modifier = Modifier.fillMaxWidth()
        ) {
            Text("Send Current Captured Nav")
        }

        Button(
            onClick = {
                context.startActivity(Intent(Settings.ACTION_NOTIFICATION_LISTENER_SETTINGS))
            },
            modifier = Modifier.fillMaxWidth()
        ) {
            Text("Open Notification Access Settings")
        }
    }
}

@Preview(showBackground = true)
@Composable
fun PhaseOneScreenPreview() {
    AndroidNavBridgeTheme {
        PhaseOneScreen()
    }
}