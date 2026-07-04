package com.marauder.controller.serial

import android.app.Notification
import android.app.NotificationChannel
import android.app.NotificationManager
import android.app.Service
import android.content.Intent
import android.os.Binder
import android.os.IBinder
import com.marauder.controller.MainApplication

class BridgeService : Service() {

    lateinit var bridge: TcpBridgeServer
        private set

    private val binder = LocalBinder()
    private var bridgeStarted = false

    inner class LocalBinder : Binder() {
        fun getService(): BridgeService = this@BridgeService
    }

    override fun onCreate() {
        super.onCreate()
        createNotificationChannel()
        bridge = TcpBridgeServer(MainApplication.repository)
    }

    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        val notif = Notification.Builder(this, CHANNEL_ID)
            .setContentTitle("Marauder Bridge")
            .setContentText("Bridge active — safe to switch to Termux")
            .setSmallIcon(android.R.drawable.ic_menu_mylocation)
            .build()
        startForeground(NOTIF_ID, notif)
        if (!bridgeStarted) {
            bridge.start()
            bridgeStarted = true
        }
        return START_STICKY
    }

    override fun onBind(intent: Intent): IBinder = binder

    override fun onDestroy() {
        super.onDestroy()
        bridge.stop()
    }

    private fun createNotificationChannel() {
        val chan = NotificationChannel(
            CHANNEL_ID, "TCP Bridge", NotificationManager.IMPORTANCE_LOW
        )
        getSystemService(NotificationManager::class.java).createNotificationChannel(chan)
    }

    companion object {
        const val CHANNEL_ID = "bridge_channel"
        const val NOTIF_ID = 1
    }
}
