package com.marauder.controller

import android.app.Application
import com.marauder.controller.serial.SerialRepository

class MainApplication : Application() {
    companion object {
        lateinit var repository: SerialRepository
            private set
    }

    override fun onCreate() {
        super.onCreate()
        repository = SerialRepository()
    }
}
