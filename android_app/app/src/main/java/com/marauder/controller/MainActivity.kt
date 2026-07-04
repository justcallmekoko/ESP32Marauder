package com.marauder.controller

import android.content.ComponentName
import android.content.Intent
import android.content.ServiceConnection
import android.os.Bundle
import android.os.IBinder
import androidx.appcompat.app.AppCompatActivity
import androidx.lifecycle.lifecycleScope
import com.marauder.controller.databinding.ActivityMainBinding
import com.marauder.controller.serial.BridgeService
import com.marauder.controller.serial.SerialRepository
import com.marauder.controller.serial.UsbConnectionManager
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.launch

class MainActivity : AppCompatActivity() {

    val repository: SerialRepository get() = MainApplication.repository

    private lateinit var usbManager: UsbConnectionManager
    private lateinit var binding: ActivityMainBinding
    private var serviceBound = false

    private val _bridgeStatus = MutableStateFlow("Bridge: starting…")
    val bridgeStatus: StateFlow<String> = _bridgeStatus.asStateFlow()

    private val serviceConnection = object : ServiceConnection {
        override fun onServiceConnected(name: ComponentName?, binder: IBinder?) {
            val service = (binder as BridgeService.LocalBinder).getService()
            lifecycleScope.launch {
                service.bridge.status.collect { _bridgeStatus.value = it }
            }
        }
        override fun onServiceDisconnected(name: ComponentName?) {
            _bridgeStatus.value = "Bridge: reconnecting…"
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)
        usbManager = UsbConnectionManager(this, MainApplication.repository)
        startForegroundService(Intent(this, BridgeService::class.java))
        serviceBound = bindService(
            Intent(this, BridgeService::class.java),
            serviceConnection,
            BIND_AUTO_CREATE
        )
    }

    override fun onResume() {
        super.onResume()
        usbManager.register()
    }

    override fun onPause() {
        super.onPause()
        usbManager.unregister()
    }

    override fun onDestroy() {
        super.onDestroy()
        if (serviceBound) {
            unbindService(serviceConnection)
            serviceBound = false
        }
        // BridgeService continues running as a ForegroundService so Termux
        // can connect even after the user navigates away from the activity.
    }
}
