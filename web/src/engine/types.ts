/**
 * TypeScript type definitions for Lookahead engine communication and telemetry.
 */

export type EngineStatus = 'uninitialized' | 'loading' | 'ready' | 'searching' | 'error';

export interface EngineScore {
  type: 'cp' | 'mate';
  value: number; // centipawns (from engine's perspective) or plies to mate
}

export interface SearchInfo {
  depth: number;
  score: EngineScore;
  nodes: number;
  nps: number;
  timeMs: number;
  pv: string[];
  pvString: string;
}

export type WorkerInMessage =
  | { type: 'init'; hashMb?: number }
  | { type: 'command'; cmd: string }
  | { type: 'position'; fen?: string; moves?: string[] }
  | { type: 'go'; depth?: number; movetime?: number }
  | { type: 'stop' }
  | { type: 'newgame' }
  | { type: 'setoption'; name: string; value: string };

export type WorkerOutMessage =
  | { type: 'status'; status: EngineStatus; message?: string }
  | { type: 'uci'; line: string }
  | { type: 'info'; info: SearchInfo }
  | { type: 'bestmove'; bestMove: string; ponderMove?: string };
