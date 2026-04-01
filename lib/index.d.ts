export declare const Priority: {
  readonly EMERG:   0;
  readonly ALERT:   1;
  readonly CRIT:    2;
  readonly ERR:     3;
  readonly WARNING: 4;
  readonly NOTICE:  5;
  readonly INFO:    6;
  readonly DEBUG:   7;
};

export declare const OpenFlags: {
  readonly LOCAL_ONLY:   number;
  readonly RUNTIME_ONLY: number;
  readonly SYSTEM:       number;
  readonly CURRENT_USER: number;
};

export declare const WaitResult: {
  readonly NOP:        0;
  readonly APPEND:     1;
  readonly INVALIDATE: 2;
};

export type PriorityValue = typeof Priority[keyof typeof Priority];
export type OpenFlagsValue = typeof OpenFlags[keyof typeof OpenFlags];

export interface JournalEntry {
  fields: Record<string, string>;
  cursor: string;
  realtimeUsec: number;
  timestamp: Date;
}

export interface FollowOptions {
  signal?: AbortSignal;
  pollUsec?: bigint;
}

/** Tek satır log yazar (sd_journal_print) */
export function print(priority: PriorityValue, message: string): void;

/** Structured log gönderir (sd_journal_sendv) */
export function send(fields: Record<string, string>): void;

export declare class JournalReader {
  private constructor(handle: unknown);

  /** Journal aç */
  static open(flags?: OpenFlagsValue): JournalReader;

  close(): void;
  seekHead(): void;
  seekTail(): void;
  seekCursor(cursor: string): void;
  seekTime(t: Date | number): void;

  /** @returns true → entry var, false → son */
  next(): boolean;
  previous(): boolean;

  /** Mevcut entry'yi oku */
  getEntry(): JournalEntry;

  /** Örnek: "PRIORITY=6" veya "_SYSTEMD_UNIT=nginx.service" */
  addMatch(match: string): void;
  flushMatches(): void;

  /** Yeni entry gelene kadar bekler. WaitResult döner. */
  wait(timeoutUsec?: bigint): number;

  /** tail -f — AbortSignal ile durdurulabilir */
  follow(
    onEntry: (entry: JournalEntry) => void,
    opts?: FollowOptions,
  ): Promise<void>;
}
