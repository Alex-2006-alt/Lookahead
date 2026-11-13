import { describe, it, expect } from 'vitest';
import { parseUciInfoLine, parseBestMoveLine } from '../src/engine/uciParser';

describe('UCI Protocol Parser', () => {
  it('parses typical search info telemetry with centipawn score', () => {
    const line = 'info depth 5 score cp 42 nodes 12500 nps 312500 time 40 pv e2e4 e7e5 g1f3 b8c6';
    const info = parseUciInfoLine(line);

    expect(info).not.toBeNull();
    expect(info?.depth).toBe(5);
    expect(info?.score).toEqual({ type: 'cp', value: 42 });
    expect(info?.nodes).toBe(12500);
    expect(info?.nps).toBe(312500);
    expect(info?.timeMs).toBe(40);
    expect(info?.pv).toEqual(['e2e4', 'e7e5', 'g1f3', 'b8c6']);
    expect(info?.pvString).toBe('e2e4 e7e5 g1f3 b8c6');
  });

  it('parses tactical mate-in-1 score correctly', () => {
    const line = 'info depth 1 score mate 1 nodes 250 nps 150000 time 2 pv h5f7';
    const info = parseUciInfoLine(line);

    expect(info).not.toBeNull();
    expect(info?.depth).toBe(1);
    expect(info?.score).toEqual({ type: 'mate', value: 1 });
    expect(info?.pv).toEqual(['h5f7']);
  });

  it('parses negative mate score correctly', () => {
    const line = 'info depth 4 score mate -2 nodes 1200 nps 200000 time 6 pv f7e8 d8h4';
    const info = parseUciInfoLine(line);

    expect(info).not.toBeNull();
    expect(info?.score).toEqual({ type: 'mate', value: -2 });
  });

  it('returns null for non-info lines', () => {
    expect(parseUciInfoLine('readyok')).toBeNull();
    expect(parseUciInfoLine('uciok')).toBeNull();
    expect(parseUciInfoLine('bestmove e2e4')).toBeNull();
  });

  it('parses bestmove line with ponder move', () => {
    const res = parseBestMoveLine('bestmove e2e4 ponder e7e5');
    expect(res).toEqual({ bestMove: 'e2e4', ponderMove: 'e7e5' });
  });

  it('parses bestmove line without ponder move', () => {
    const res = parseBestMoveLine('bestmove h5f7');
    expect(res).toEqual({ bestMove: 'h5f7', ponderMove: undefined });
  });

  it('returns null for malformed bestmove lines', () => {
    expect(parseBestMoveLine('bestmove')).toBeNull();
    expect(parseBestMoveLine('info depth 1')).toBeNull();
  });
});
