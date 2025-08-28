import { Component, createSignal, onMount, onCleanup } from "solid-js";
import { Slider } from "@kobalte/core/slider";
import { Parameters } from "../lib/parameter-store";
import {
  formatParameterValue,
  sendParameterValue,
  beginParameterAutomation,
  endParameterAutomation,
  onParameterUpdate,
} from "../lib/iplug-bridge";

interface ParameterSliderProps {
  paramIdx: number;
  class?: string;
}

export const ParameterSlider: Component<ParameterSliderProps> = (props) => {
  const param = Parameters[props.paramIdx];

  if (!param || param.type !== "continuous") {
    console.error(
      `Invalid parameter index or not continuous: ${props.paramIdx}`,
    );
    return null;
  }

  // Local state for this parameter
  const [value, setValue] = createSignal(param.default);
  let isDragging = false;

  onMount(() => {
    // Subscribe to parameter updates from plugin
    const unsubscribe = onParameterUpdate((paramIdx, displayValue) => {
      // Only update if it's our parameter and we're not dragging
      if (paramIdx === props.paramIdx && !isDragging) {
        setValue(displayValue);
      }
    });

    // Clean up subscription
    onCleanup(unsubscribe);
  });

  // Handle value changes from user
  const handleChange = (values: number[]) => {
    const newValue = values[0];
    setValue(newValue);
    sendParameterValue(props.paramIdx, newValue);
  };

  // Handle automation
  const handleInteractionStart = () => {
    isDragging = true;
    beginParameterAutomation(props.paramIdx);
  };

  const handleInteractionEnd = () => {
    isDragging = false;
    endParameterAutomation(props.paramIdx);
  };

  return (
    <Slider
      class={props.class || "w-full"}
      value={[value()]}
      onChange={handleChange}
      minValue={param.min}
      maxValue={param.max}
      step={param.step}
    >
      <div class="mb-2 flex justify-between">
        <Slider.Label>{param.name}</Slider.Label>
        <Slider.ValueLabel>
          {formatParameterValue(props.paramIdx, value())}
        </Slider.ValueLabel>
      </div>

      <Slider.Track
        class="relative h-12 w-full bg-gray-300 rounded"
        onPointerDown={handleInteractionStart}
        onPointerUp={handleInteractionEnd}
      >
        <Slider.Fill class="absolute h-full bg-gray-600" />
        <Slider.Thumb class="block w-4 h-4 bg-white border border-black">
          <Slider.Input />
        </Slider.Thumb>
      </Slider.Track>
    </Slider>
  );
};
