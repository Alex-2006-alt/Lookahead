/**
 * Parser for Universal Chess Interface (UCI) protocol strings and search telemetry.
 */

import { SearchInfo, EngineScore } from './types';

export function parseUciInfoLine(line: string): SearchInfo | null {
  const trimmed = line.trim();
  if (!trimmed.startsWith('info')) {
    return null;
  }

  const tokens = trimmed.split(/\s+/);
  let depth = 0;
  let score: EngineScore = { type: 'cp', value: 0 };
  let nodes = 0;
  let nps = 0;
  let timeMs = 0;
  const pv: string[] = [];
  let inPv = false;

  for (let i = 1; i < tokens.length; i++) {
    const tok = tokens[i];

    if (inPv) {
      pv.push(tok);
      continue;
    }

    if (tok === 'depth' && i + 1 < tokens.length) {
      depth = parseInt(tokens[++i], 10) || 0;
    } else if (tok === 'score' && i + 2 < tokens.length) {
      const scoreType = tokens[++i];
      const scoreVal = parseInt(tokens[++i], 10) || 0;
      if (scoreType === 'cp') {
        score = { type: 'cp', value: scoreVal };
      } else if (scoreType === 'mate') {
        score = { type: 'mate', value: scoreVal };
      }
    } else if (tok === 'nodes' && i + 1 < tokens.length) {
      nodes = parseInt(tokens[++i], 10) || 0;
    } else if (tok === 'nps' && i + 1 < tokens.length) {
      nps = parseInt(tokens[++i], 10) || 0;
    } else if (tok === 'time' && i + 1 < tokens.length) {
      timeMs = parseInt(tokens[++i], 10) || 0;
    } else if (tok === 'pv') {
      inPv = true;
    }
  }

  return {
    depth,
    score,
    nodes,
    nps,
    timeMs,
    pv,
    pvString: pv.join(' '),
  };
}

export function parseBestMoveLine(line: string): { bestMove: string; ponderMove?: string } | null {
  const trimmed = line.trim();
  if (!trimmed.startsWith('bestmove')) {
    return null;
  }

  const tokens = trimmed.split(/\s+/);
  if (tokens.length < 2) {
    return null;
  }

  const bestMove = tokens[1];
  let ponderMove: string | undefined;

  if (tokens.length >= 4 && tokens[2] === 'ponder') {
    ponderMove = tokens[3];
  }

  return { bestMove, ponderMove };
}
