package com.marauder.controller.ui.connection

import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import androidx.fragment.app.Fragment
import androidx.fragment.app.activityViewModels
import androidx.lifecycle.Lifecycle
import androidx.lifecycle.lifecycleScope
import androidx.lifecycle.repeatOnLifecycle
import androidx.navigation.fragment.findNavController
import com.marauder.controller.MainActivity
import com.marauder.controller.R
import com.marauder.controller.databinding.FragmentConnectionBinding
import com.marauder.controller.ui.terminal.TerminalViewModel
import kotlinx.coroutines.launch

class ConnectionFragment : Fragment() {

    private var _binding: FragmentConnectionBinding? = null
    private val binding get() = _binding!!

    private val vm: TerminalViewModel by activityViewModels {
        TerminalViewModel.Factory((requireActivity() as MainActivity).repository)
    }

    override fun onCreateView(
        inflater: LayoutInflater, container: ViewGroup?, savedInstanceState: Bundle?
    ): View {
        _binding = FragmentConnectionBinding.inflate(inflater, container, false)
        return binding.root
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        viewLifecycleOwner.lifecycleScope.launch {
            repeatOnLifecycle(Lifecycle.State.STARTED) {
                vm.isConnected.collect { connected ->
                    if (connected) {
                        val dest = findNavController().currentDestination?.id
                        if (dest == R.id.connectionFragment) {
                            findNavController().navigate(R.id.action_connection_to_terminal)
                        }
                    }
                }
            }
        }
    }

    override fun onDestroyView() {
        super.onDestroyView()
        _binding = null
    }
}
