export default function App() {
  return (
    <div className="flex flex-col min-h-screen bg-surface-darker text-gray-100">
      <header className="h-14 border-b border-surface-border px-6 flex items-center justify-between bg-surface-dark">
        <div className="flex items-center gap-3">
          <span className="text-2xl">♟</span>
          <h1 className="text-lg font-bold tracking-tight">Lookahead</h1>
          <span className="text-xs bg-blue-900/60 text-blue-300 border border-blue-700/50 px-2 py-0.5 rounded font-mono">
            WASM C++20
          </span>
        </div>
      </header>
      <main className="flex-1 p-6 flex items-center justify-center">
        <p className="text-gray-400">Lookahead Chess Engine Web Interface</p>
      </main>
    </div>
  );
}

