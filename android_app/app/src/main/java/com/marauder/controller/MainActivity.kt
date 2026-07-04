package com.marauder.controller

import android.os.Bundle
import androidx.appcompat.app.AppCompatActivity
import com.marauder.controller.databinding.ActivityMainBinding
import com.marauder.controller.serial.SerialRepository
import com.marauder.controller.serial.TcpBridgeServer
import com.marauder.controller.serial.UsbConnectionManager

class MainActivity : AppCompatActivity() {

    val repository = SerialRepository()
    private lateinit var usbManager: UsbConnectionManager
    private lateinit var binding: ActivityMainBinding
    private lateinit var tcpBridge: TcpBridgeServer

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)
        usbManager = UsbConnectionManager(this, repository)
        tcpBridge = TcpBridgeServer(repository)
        tcpBridge.start()
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
        tcpBridge.stop()
        repository.close()
    }
}
