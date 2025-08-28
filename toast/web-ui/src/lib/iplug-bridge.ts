/**
 * iPlug2 Bridge - Communication Layer
 * Handles all message protocol between SolidJS UI and iPlug2 plugin
 */

import { Parameters } from "./parameter-store";

// Callback type for parameter updates
type ParameterUpdateCallback = (paramIdx: number, displayValue: number) => void;
type MeterUpdateCallback = (levels: number[]) => void;

// Store callbacks for parameter and meter updates
let parameterUpdateCallbacks: ParameterUpdateCallback[] = [];
let meterUpdateCallbacks: MeterUpdateCallback[] = [];

/**
 * Convert normalized value (0-1) from plugin to display value
 */
export function normalizedToDisplay(
  paramIdx: number,
  normalizedValue: number,
): number {
  const param = Parameters[paramIdx];
  if (!param) return normalizedValue;

  if (param.type === "boolean") {
    return normalizedValue > 0.5 ? 1 : 0;
  }

  return normalizedValue * (param.max - param.min) + param.min;
}

/**
 * Convert display value to normalized (0-1) for plugin
 */
export function displayToNormalized(
  paramIdx: number,
  displayValue: number,
): number {
  const param = Parameters[paramIdx];
  if (!param) return displayValue;

  if (param.type === "boolean") {
    return displayValue > 0.5 ? 1.0 : 0.0;
  }

  return (displayValue - param.min) / (param.max - param.min);
}

/**
 * Format parameter value for display
 */
export function formatParameterValue(
  paramIdx: number,
  displayValue: number,
): string {
  const param = Parameters[paramIdx];
  if (!param) return displayValue.toString();

  if (param.type === "boolean") {
    return displayValue > 0.5 ? "ON" : "OFF";
  }

  const decimals = param.step < 1 ? Math.abs(Math.log10(param.step)) : 0;
  const formatted = displayValue.toFixed(decimals);
  return param.unit ? `${formatted} ${param.unit}` : formatted;
}

/**
 * Send parameter value to plugin
 */
export function sendParameterValue(
  paramIdx: number,
  displayValue: number,
): void {
  const normalized = displayToNormalized(paramIdx, displayValue);

  if ((globalThis as any).IPlugSendMsg) {
    (globalThis as any).IPlugSendMsg({
      msg: "SPVFUI",
      paramIdx: paramIdx,
      value: normalized,
    });
  }
}

/**
 * Begin parameter automation
 */
export function beginParameterAutomation(paramIdx: number): void {
  if ((globalThis as any).IPlugSendMsg) {
    (globalThis as any).IPlugSendMsg({
      msg: "BPCFUI",
      paramIdx: paramIdx,
    });
  }
}

/**
 * End parameter automation
 */
export function endParameterAutomation(paramIdx: number): void {
  if ((globalThis as any).IPlugSendMsg) {
    (globalThis as any).IPlugSendMsg({
      msg: "EPCFUI",
      paramIdx: paramIdx,
    });
  }
}

/**
 * Register callback for parameter updates
 */
export function onParameterUpdate(
  callback: ParameterUpdateCallback,
): () => void {
  parameterUpdateCallbacks.push(callback);

  // Return unsubscribe function
  return () => {
    const index = parameterUpdateCallbacks.indexOf(callback);
    if (index > -1) {
      parameterUpdateCallbacks.splice(index, 1);
    }
  };
}

/**
 * Register callback for meter updates
 */
export function onMeterUpdate(callback: MeterUpdateCallback): () => void {
  meterUpdateCallbacks.push(callback);

  // Return unsubscribe function
  return () => {
    const index = meterUpdateCallbacks.indexOf(callback);
    if (index > -1) {
      meterUpdateCallbacks.splice(index, 1);
    }
  };
}

/**
 * Initialize the iPlug2 communication bridge
 * Must be called once when the app starts
 */
export function initializeBridge(): boolean {
  console.log("Initializing iPlug2 Bridge");

  // Set up global message handler for parameter updates from plugin
  (globalThis as any).SPVFD = (paramIdx: number, normalizedValue: number) => {
    console.log(
      `SPVFD received: param=${paramIdx}, normalized=${normalizedValue}`,
    );

    const displayValue = normalizedToDisplay(paramIdx, normalizedValue);

    // Notify all registered callbacks
    parameterUpdateCallbacks.forEach((callback) => {
      callback(paramIdx, displayValue);
    });
  };

  // Set up meter data handler
  (globalThis as any).SAMFD = (dataArray: number[]) => {
    console.log("Meter levels:", dataArray);

    // Notify all registered callbacks
    meterUpdateCallbacks.forEach((callback) => {
      callback(dataArray);
    });
  };

  // Set up other required handlers
  (globalThis as any).SCMFD = (
    ctrlTag: number,
    msgTag: number,
    dataSize: number,
    data: any,
  ) => {
    console.log("Control message:", { ctrlTag, msgTag, dataSize, data });
  };

  (globalThis as any).SMMFD = (
    msgTag: number,
    dataSize: number,
    midiData: any,
  ) => {
    console.log("MIDI message:", { msgTag, dataSize, midiData });
  };

  (globalThis as any).SSMFD = (
    msgTag: number,
    dataSize: number,
    sysexData: any,
  ) => {
    console.log("SysEx message:", { msgTag, dataSize, sysexData });
  };

  // Check if IPlugSendMsg is available
  if (!(globalThis as any).IPlugSendMsg) {
    console.warn("IPlugSendMsg not available - running outside plugin context");

    // Create mock function for testing
    (globalThis as any).IPlugSendMsg = (msg: any) => {
      console.log("Mock IPlugSendMsg:", msg);

      // Simulate parameter echo for testing
      if (msg.msg === "SPVFUI" && typeof msg.value === "number") {
        setTimeout(() => {
          (globalThis as any).SPVFD?.(msg.paramIdx, msg.value);
        }, 10);
      }
    };

    return false; // Not in plugin context
  } else {
    console.log("IPlugSendMsg is available - plugin communication ready");
    return true; // In plugin context
  }
}

/**
 * Clean up bridge (call on app unmount)
 */
export function cleanupBridge(): void {
  delete (globalThis as any).SPVFD;
  delete (globalThis as any).SAMFD;
  delete (globalThis as any).SCMFD;
  delete (globalThis as any).SMMFD;
  delete (globalThis as any).SSMFD;

  // Clear callbacks
  parameterUpdateCallbacks = [];
  meterUpdateCallbacks = [];
}
