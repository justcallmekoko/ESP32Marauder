package com.marauder.controller.ui.terminal

import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.view.inputmethod.EditorInfo
import androidx.fragment.app.Fragment
import androidx.fragment.app.activityViewModels
import androidx.lifecycle.Lifecycle
import androidx.lifecycle.lifecycleScope
import androidx.lifecycle.repeatOnLifecycle
import androidx.navigation.fragment.findNavController
import androidx.recyclerview.widget.LinearLayoutManager
import com.marauder.controller.MainActivity
import com.marauder.controller.R
import com.marauder.controller.databinding.FragmentTerminalBinding
import kotlinx.coroutines.launch

class TerminalFragment : Fragment() {

    private var _binding: FragmentTerminalBinding? = null
    private val binding get() = _binding!!

    private val vm: TerminalViewModel by activityViewModels {
        TerminalViewModel.Factory((requireActivity() as MainActivity).repository)
    }

    private val terminalAdapter = TerminalAdapter()
    private val quickAdapter = QuickActionAdapter { action -> vm.sendCommand(action.command) }

    override fun onCreateView(
        inflater: LayoutInflater, container: ViewGroup?, savedInstanceState: Bundle?
    ): View {
        _binding = FragmentTerminalBinding.inflate(inflater, container, false)
        return binding.root
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        val llm = LinearLayoutManager(requireContext()).apply { stackFromEnd = true }
        binding.rvTerminal.layoutManager = llm
        binding.rvTerminal.adapter = terminalAdapter

        binding.rvQuickActions.layoutManager =
            LinearLayoutManager(requireContext(), LinearLayoutManager.HORIZONTAL, false)
        binding.rvQuickActions.adapter = quickAdapter
        quickAdapter.submitList(vm.quickActions)

        binding.btnSend.setOnClickListener { submitInput() }
        binding.etInput.setOnEditorActionListener { _, actionId, _ ->
            if (actionId == EditorInfo.IME_ACTION_SEND) { submitInput(); true } else false
        }
        binding.btnClear.setOnClickListener { vm.clearTerminal() }

        viewLifecycleOwner.lifecycleScope.launch {
            repeatOnLifecycle(Lifecycle.State.STARTED) {
                launch {
                    vm.lines.collect { lines ->
                        terminalAdapter.submitList(lines.toList()) {
                            if (terminalAdapter.itemCount > 0) {
                                binding.rvTerminal.scrollToPosition(terminalAdapter.itemCount - 1)
                            }
                        }
                    }
                }
                launch {
                    vm.commandPending.collect { pending ->
                        binding.etInput.isEnabled = !pending
                        binding.btnSend.isEnabled = !pending
                    }
                }
                launch {
                    vm.isConnected.collect { connected ->
                        if (!connected) {
                            val dest = findNavController().currentDestination?.id
                            if (dest == R.id.terminalFragment) {
                                findNavController().navigate(R.id.action_terminal_to_connection)
                            }
                        }
                    }
                }
            }
        }
    }

    private fun submitInput() {
        val cmd = binding.etInput.text.toString().trim()
        if (cmd.isNotEmpty()) {
            vm.sendCommand(cmd)
            binding.etInput.text?.clear()
        }
    }

    override fun onDestroyView() {
        super.onDestroyView()
        _binding = null
    }
}
