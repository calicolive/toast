import type { Component } from "solid-js";

const App: Component = () => {
  return (
    <>
      <main class="min-h-screen w-screen bg-emerald-300">
        {/* this is where the top menu will be. this will be where presets are stored. */}
        <header class="container mx-auto max-w-2xl">
          <nav class="sticky top-0 flex justify-between p-6 text-center ">
            <div class="text-xl font-bold">Toast</div>
            <div>Preset menu</div>
            <div></div>
          </nav>
        </header>
        {/* this is where the parameters will go */}
        <div></div>
      </main>
    </>
  );
};

export default App;
