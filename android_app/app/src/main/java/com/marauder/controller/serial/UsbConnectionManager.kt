package com.marauder.controller.serial

import android.app.PendingIntent
import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.hardware.usb.UsbDevice
import android.hardware.usb.UsbManager
import androidx.core.content.ContextCompat
import com.hoho.android.usbserial.driver.UsbSerialProber

class UsbConnectionManager(
    private val context: Context,
    private val repository: SerialRepository,
) {
    private val usbManager = context.getSystemService(Context.USB_SERVICE) as UsbManager
    private val ACTION_USB_PERMISSION = "com.marauder.controller.USB_PERMISSION"

    private val receiver = object : BroadcastReceiver() {
        override fun onReceive(ctx: Context, intent: Intent) {
            when (intent.action) {
                UsbManager.ACTION_USB_DEVICE_ATTACHED -> {
                    @Suppress("DEPRECATION")
                    val device: UsbDevice? = intent.getParcelableExtra(UsbManager.EXTRA_DEVICE)
                    device?.let { requestPermission(it) }
                }
                UsbManager.ACTION_USB_DEVICE_DETACHED -> {
                    if (repository.isConnected.value) repository.disconnect()
                }
                ACTION_USB_PERMISSION -> {
                    val granted = intent.getBooleanExtra(UsbManager.EXTRA_PERMISSION_GRANTED, false)
                    @Suppress("DEPRECATION")
                    val device: UsbDevice? = intent.getParcelableExtra(UsbManager.EXTRA_DEVICE)
                    if (granted && device != null) openDevice(device)
                }
            }
        }
    }

    fun register() {
        val filter = IntentFilter().apply {
            addAction(UsbManager.ACTION_USB_DEVICE_ATTACHED)
            addAction(UsbManager.ACTION_USB_DEVICE_DETACHED)
            addAction(ACTION_USB_PERMISSION)
        }
        // RECEIVER_NOT_EXPORTED required on Android 13+ for receivers with custom actions
        ContextCompat.registerReceiver(context, receiver, filter, ContextCompat.RECEIVER_NOT_EXPORTED)

        // Connect to already-attached device (e.g. app started while device was plugged in)
        if (!repository.isConnected.value) {
            usbManager.deviceList.values.firstOrNull { isMarauderDevice(it) }?.let { device ->
                if (usbManager.hasPermission(device)) openDevice(device)
                else requestPermission(device)
            }
        }
    }

    fun unregister() {
        runCatching { context.unregisterReceiver(receiver) }
    }

    private fun requestPermission(device: UsbDevice) {
        if (!isMarauderDevice(device)) return
        val flags = PendingIntent.FLAG_UPDATE_CURRENT or PendingIntent.FLAG_IMMUTABLE
        val permIntent = PendingIntent.getBroadcast(context, 0, Intent(ACTION_USB_PERMISSION), flags)
        usbManager.requestPermission(device, permIntent)
    }

    private fun openDevice(device: UsbDevice) {
        val driver = UsbSerialProber.getDefaultProber().probeDevice(device) ?: return
        val connection = usbManager.openDevice(device) ?: return
        repository.connect(driver.ports[0], connection)
    }

    private fun isMarauderDevice(device: UsbDevice) = device.vendorId == 0x1A86
}
