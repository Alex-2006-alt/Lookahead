/**
 * Custom React hook managing Lookahead Web Worker instance lifecycle,
 * search execution, real-time telemetry streaming, and UCI command routing.
 */

import { useState, useEffect, useRef, useCallback } from 'react';
import { EngineStatus, SearchInfo, WorkerInMessage, WorkerOutMessage } from './types';

export interface UseEngineReturn {
  status: EngineStatus;
  statusMessage?: string;
  isSearching: boolean;
  searchInfo: SearchInfo | null;
  lastBestMove: string | null;
  lastPonderMove: string | null;
  uciLogs: string[];
  startSearch: (params?: { fen?: string; moves?: string[]; depth?: number; movetime?: number }) => void;
  stopSearch: () => void;
  newGame: () => void;
  sendCommand: (cmd: string) => void;
  setOption: (name: string, value: string) => void;
  clearLogs: () => void;
}

export function useEngine(hashMb = 16): UseEngineReturn {
  const [status, setStatus] = useState<EngineStatus>('uninitialized');
  const [statusMessage, setStatusMessage] = useState<string | undefined>(undefined);
  const [searchInfo, setSearchInfo] = useState<SearchInfo | null>(null);
  const [lastBestMove, setLastBestMove] = useState<string | null>(null);
  const [lastPonderMove, setLastPonderMove] = useState<string | null>(null);
  const [uciLogs, setUciLogs] = useState<string[]>([]);
  const workerRef = useRef<Worker | null>(null);

  useEffect(() => {
    const worker = new Worker(new URL('./engineWorker.ts', import.meta.url), {
      type: 'module',
    });
    workerRef.current = worker;

    worker.onmessage = (event: MessageEvent<WorkerOutMessage>) => {
      const msg = event.data;

      switch (msg.type) {
        case 'status':
          setStatus(msg.status);
          setStatusMessage(msg.message);
          break;

        case 'info':
          setSearchInfo(msg.info);
          break;

        case 'bestmove':
          setLastBestMove(msg.bestMove);
          setLastPonderMove(msg.ponderMove ?? null);
          break;

        case 'uci':
          setUciLogs((prev) => [...prev.slice(-100), msg.line]);
          break;
      }
    };

    worker.postMessage({ type: 'init', hashMb } as WorkerInMessage);

    return () => {
      worker.terminate();
      workerRef.current = null;
    };
  }, [hashMb]);

  const postToWorker = useCallback((msg: WorkerInMessage) => {
    if (workerRef.current) {
      workerRef.current.postMessage(msg);
    }
  }, []);

  const startSearch = useCallback(
    (params?: { fen?: string; moves?: string[]; depth?: number; movetime?: number }) => {
      setLastBestMove(null);
      setLastPonderMove(null);
      setSearchInfo(null);

      if (params?.fen || params?.moves) {
        postToWorker({
          type: 'position',
          fen: params.fen,
          moves: params.moves,
        });
      }

      postToWorker({
        type: 'go',
        depth: params?.depth,
        movetime: params?.movetime,
      });
    },
    [postToWorker]
  );

  const stopSearch = useCallback(() => {
    postToWorker({ type: 'stop' });
  }, [postToWorker]);

  const newGame = useCallback(() => {
    setLastBestMove(null);
    setLastPonderMove(null);
    setSearchInfo(null);
    postToWorker({ type: 'newgame' });
  }, [postToWorker]);

  const sendCommand = useCallback(
    (cmd: string) => {
      postToWorker({ type: 'command', cmd });
    },
    [postToWorker]
  );

  const setOption = useCallback(
    (name: string, value: string) => {
      postToWorker({ type: 'setoption', name, value });
    },
    [postToWorker]
  );

  const clearLogs = useCallback(() => {
    setUciLogs([]);
  }, []);

  return {
    status,
    statusMessage,
    isSearching: status === 'searching',
    searchInfo,
    lastBestMove,
    lastPonderMove,
    uciLogs,
    startSearch,
    stopSearch,
    newGame,
    sendCommand,
    setOption,
    clearLogs,
  };
}
