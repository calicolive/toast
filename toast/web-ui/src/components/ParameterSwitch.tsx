import { Component, createSignal, onMount, onCleanup } from "solid-js";
import { Switch } from "@kobalte/core/switch";
import { Parameters } from "../lib/parameter-store";
import {
  sendParameterValue,
  beginParameterAutomation,
  endParameterAutomation,
  onParameterUpdate,
} from "../lib/iplug-bridge";

interface ParameterSwitchProps {
  paramIdx: number;
  class?: string;
}

export const ParameterSwitch: Component<ParameterSwitchProps> = (props) => {
  const param = Parameters[props.paramIdx];

  if (!param || param.type !== "boolean") {
    console.error(`Invalid parameter index or not boolean: ${props.paramIdx}`);
    return null;
  }

  // Local state for this parameter
  const [checked, setChecked] = createSignal(param.default === 1);

  onMount(() => {
    // Subscribe to parameter updates from plugin
    const unsubscribe = onParameterUpdate((paramIdx, displayValue) => {
      if (paramIdx === props.paramIdx) {
        setChecked(displayValue > 0.5);
      }
    });

    // Clean up subscription
    onCleanup(unsubscribe);
  });

  // Handle value changes from user
  const handleChange = (isChecked: boolean) => {
    setChecked(isChecked);

    // For boolean parameters, send automation immediately
    beginParameterAutomation(props.paramIdx);
    sendParameterValue(props.paramIdx, isChecked ? 1.0 : 0.0);
    endParameterAutomation(props.paramIdx);
  };

  return (
    <Switch
      checked={checked()}
      onChange={handleChange}
      class={props.class || "flex items-center gap-2"}
    >
      <Switch.Label class="text-sm">{param.name}</Switch.Label>
      <Switch.Control class="w-10 h-6 bg-gray-300 rounded-full p-1">
        <Switch.Thumb class="block w-4 h-4 bg-white rounded-full data-[checked]:translate-x-4 transition-transform" />
      </Switch.Control>
    </Switch>
  );
};
