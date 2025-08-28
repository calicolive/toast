/**
 * Parameter Store - All parameter definitions and metadata
 */

// Parameter indices matching iPlug
export const ParameterIndex = {
  DRIVE: 0, // Input gain
  THD_AMOUNT: 1, // Drive/saturation amount
  DYNAMICS: 2, // Dynamic THD modulation
  THRESHOLD: 3, // Dynamics threshold
  ATTACK: 4, // Envelope attack time
  RELEASE: 5, // Envelope release time
  CURVE: 6, // Envelope curve shape
  MIX: 7, // Dry/wet mix
  OUTPUT: 8, // Output gain
  LINK_GAIN: 9, // Link input/output gains (boolean)
} as const;

// Parameter type definitions
export interface ParameterConfig {
  name: string;
  displayName: string;
  min: number;
  max: number;
  default: number;
  step: number;
  unit: string;
  type: "continuous" | "boolean";
  scaling?: "linear" | "exponential" | "discrete";
  group: string;
}

// All parameters
export const Parameters: Record<number, ParameterConfig> = {
  [ParameterIndex.DRIVE]: {
    name: "Input",
    displayName: "INPUT",
    min: -12.0,
    max: 12.0,
    default: 0.0,
    step: 0.1,
    unit: "dB",
    type: "continuous",
    scaling: "linear",
    group: "input",
  },
  [ParameterIndex.THD_AMOUNT]: {
    name: "Drive",
    displayName: "DRIVE",
    min: 0.0,
    max: 100.0,
    default: 30.0,
    step: 0.1,
    unit: "%",
    type: "continuous",
    scaling: "linear",
    group: "saturation",
  },
  [ParameterIndex.DYNAMICS]: {
    name: "Dynamics",
    displayName: "DYNAMICS",
    min: -100.0,
    max: 100.0,
    default: 0.0,
    step: 1.0,
    unit: "%",
    type: "continuous",
    scaling: "linear",
    group: "dynamics",
  },
  [ParameterIndex.THRESHOLD]: {
    name: "Threshold",
    displayName: "THRESHOLD",
    min: -60.0,
    max: 0.0,
    default: -20.0,
    step: 0.5,
    unit: "dB",
    type: "continuous",
    scaling: "linear",
    group: "dynamics",
  },
  [ParameterIndex.ATTACK]: {
    name: "Attack",
    displayName: "ATTACK",
    min: 0.5,
    max: 50.0,
    default: 1.0,
    step: 0.5,
    unit: "ms",
    type: "continuous",
    scaling: "exponential",
    group: "dynamics",
  },
  [ParameterIndex.RELEASE]: {
    name: "Release",
    displayName: "RELEASE",
    min: 10.0,
    max: 500.0,
    default: 120.0,
    step: 1.0,
    unit: "ms",
    type: "continuous",
    scaling: "exponential",
    group: "dynamics",
  },
  [ParameterIndex.CURVE]: {
    name: "Curve",
    displayName: "CURVE",
    min: 0.0,
    max: 100.0,
    default: 50.0,
    step: 1.0,
    unit: "%",
    type: "continuous",
    scaling: "linear",
    group: "dynamics",
  },
  [ParameterIndex.MIX]: {
    name: "Mix",
    displayName: "MIX",
    min: 0.0,
    max: 100.0,
    default: 100.0,
    step: 0.1,
    unit: "%",
    type: "continuous",
    scaling: "linear",
    group: "output",
  },
  [ParameterIndex.OUTPUT]: {
    name: "Output",
    displayName: "OUTPUT",
    min: -12.0,
    max: 12.0,
    default: 0.0,
    step: 0.1,
    unit: "dB",
    type: "continuous",
    scaling: "linear",
    group: "output",
  },
  [ParameterIndex.LINK_GAIN]: {
    name: "Link",
    displayName: "LINK",
    min: 0,
    max: 1,
    default: 1,
    step: 1,
    unit: "",
    type: "boolean",
    scaling: "discrete",
    group: "output",
  },
};

// checks to see if parameter is boolean
export function isBoolean(paramIdx: number): boolean {
  return Parameters[paramIdx]?.type === "boolean";
}
