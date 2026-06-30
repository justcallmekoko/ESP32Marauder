package com.marauder.controller.ui.terminal

import android.graphics.Color
import android.view.LayoutInflater
import android.view.ViewGroup
import androidx.recyclerview.widget.DiffUtil
import androidx.recyclerview.widget.ListAdapter
import androidx.recyclerview.widget.RecyclerView
import com.marauder.controller.databinding.ItemTerminalLineBinding
import com.marauder.controller.model.LineType
import com.marauder.controller.model.TerminalLine

class TerminalAdapter : ListAdapter<TerminalLine, TerminalAdapter.ViewHolder>(DIFF) {

    inner class ViewHolder(val binding: ItemTerminalLineBinding) :
        RecyclerView.ViewHolder(binding.root)

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): ViewHolder {
        val binding = ItemTerminalLineBinding.inflate(
            LayoutInflater.from(parent.context), parent, false
        )
        return ViewHolder(binding)
    }

    override fun onBindViewHolder(holder: ViewHolder, position: Int) {
        val line = getItem(position)
        holder.binding.tvLine.apply {
            text = line.text
            setTextColor(
                when (line.type) {
                    LineType.SENT     -> Color.parseColor("#00FF41")   // matrix green
                    LineType.RECEIVED -> Color.WHITE
                    LineType.SYSTEM   -> Color.parseColor("#FFD700")   // gold
                }
            )
        }
    }

    companion object {
        private val DIFF = object : DiffUtil.ItemCallback<TerminalLine>() {
            override fun areItemsTheSame(a: TerminalLine, b: TerminalLine) = a === b
            override fun areContentsTheSame(a: TerminalLine, b: TerminalLine) = a == b
        }
    }
}
