package com.marauder.controller.ui.terminal

import android.view.LayoutInflater
import android.view.ViewGroup
import androidx.recyclerview.widget.DiffUtil
import androidx.recyclerview.widget.ListAdapter
import androidx.recyclerview.widget.RecyclerView
import com.marauder.controller.databinding.ItemQuickActionBinding
import com.marauder.controller.model.QuickAction

class QuickActionAdapter(
    private val onClick: (QuickAction) -> Unit,
) : ListAdapter<QuickAction, QuickActionAdapter.ViewHolder>(DIFF) {

    inner class ViewHolder(val binding: ItemQuickActionBinding) :
        RecyclerView.ViewHolder(binding.root)

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): ViewHolder {
        val binding = ItemQuickActionBinding.inflate(
            LayoutInflater.from(parent.context), parent, false
        )
        return ViewHolder(binding)
    }

    override fun onBindViewHolder(holder: ViewHolder, position: Int) {
        val action = getItem(position)
        holder.binding.btnQuickAction.text = action.label
        holder.binding.btnQuickAction.setOnClickListener { onClick(action) }
    }

    companion object {
        private val DIFF = object : DiffUtil.ItemCallback<QuickAction>() {
            override fun areItemsTheSame(a: QuickAction, b: QuickAction) = a.command == b.command
            override fun areContentsTheSame(a: QuickAction, b: QuickAction) = a == b
        }
    }
}
