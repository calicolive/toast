import { Component, onMount, onCleanup } from "solid-js";
import { ParameterSlider } from "./components/ParameterSlider";
import { ParameterIndex } from "./lib/parameter-store";
import { initializeBridge, cleanupBridge } from "./lib/iplug-bridge";

const App: Component = () => {
  onMount(() => {
    // Initialize the bridge once for the entire app
    const isInPlugin = initializeBridge();
    console.log(
      isInPlugin ? "Running in plugin context" : "Running standalone",
    );
  });

  onCleanup(() => {
    // Clean up when app unmounts
    cleanupBridge();
  });

  return (
    <>
      <main class="min-h-screen w-screen overflow-hidden bg-indigo-100">
        {/* this is where the top menu will be. this will be where presets are stored. */}
        <header class="container mx-auto max-w-2xl">
          <nav class="sticky top-0 flex justify-between p-6 text-center">
            <div class="text-xl font-bold">Toast</div>
            <div>Preset menu</div>
            <div></div>
          </nav>
        </header>

        {/* this is where the parameters will go */}
        <div class="container mx-auto max-w-2xl p-6">
          <div class="p-4 ">
            {/* Input Gain Slider */}

            <ParameterSlider paramIdx={ParameterIndex.DRIVE} />
          </div>
        </div>
      </main>
    </>
  );
};

export default App;
